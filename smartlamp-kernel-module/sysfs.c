#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102");
MODULE_LICENSE("GPL");


#define MAX_RECV_LINE 100 // Tamanho máximo de uma linha de resposta do dispositvo USB


static struct usb_device *smartlamp_device;        // Referência para o dispositivo USB
static uint usb_in, usb_out;                       // Endereços das portas de entrada e saida da USB
static char *usb_in_buffer, *usb_out_buffer;       // Buffers de entrada e saída da USB
static int usb_max_size;                           // Tamanho máximo de uma mensagem USB

#define VENDOR_ID   0x10c4 /* Encontre o VendorID  do smartlamp */
#define PRODUCT_ID  0xea60 /* Encontre o ProductID do smartlamp */
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id); // Executado quando o dispositivo é conectado na USB
static void usb_disconnect(struct usb_interface *ifce);                           // Executado quando o dispositivo USB é desconectado da USB
static int  usb_read_serial(void);

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é lido (e.g., cat /sys/kernel/smartlamp/led)
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff);
// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é escrito (e.g., echo "100" | sudo tee -a /sys/kernel/smartlamp/led)
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count);

// Variáveis para criar os arquivos no /sys/kernel/smartlamp/{led, ldr}
static struct kobj_attribute  led_attribute = __ATTR(led, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  ldr_attribute = __ATTR(ldr, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct attribute      *attrs[]       = { &led_attribute.attr, &ldr_attribute.attr, NULL };
static struct attribute_group attr_group    = { .attrs = attrs };
static struct kobject        *sys_obj;

MODULE_DEVICE_TABLE(usb, id_table);

bool ignore = true;
// A variável LDR_value não é mais necessária aqui
// int LDR_value = 0;

static struct usb_driver smartlamp_driver = {
    .name        = "smartlamp",     // Nome do driver
    .probe       = usb_probe,       // Executado quando o dispositivo é conectado na USB
    .disconnect  = usb_disconnect,  // Executado quando o dispositivo é desconectado na USB
    .id_table    = id_table,        // Tabela com o VendorID e ProductID do dispositivo
};

module_usb_driver(smartlamp_driver);

// Executado quando o dispositivo é conectado na USB
static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    struct usb_endpoint_descriptor *usb_endpoint_in, *usb_endpoint_out;

    printk(KERN_INFO "SmartLamp: Dispositivo conectado ...\n");

    // Cria arquivos do /sys/kernel/smartlamp/*
    sys_obj = kobject_create_and_add("smartlamp", kernel_kobj);
    if (sysfs_create_group(sys_obj, &attr_group))
        printk(KERN_ERR "SmartLamp: Erro ao criar grupo de atributos no sysfs.\n");

    // Detecta portas e aloca buffers de entrada e saída de dados na USB
    smartlamp_device = interface_to_usbdev(interface);
    if(usb_find_common_endpoints(interface->cur_altsetting, &usb_endpoint_in, &usb_endpoint_out, NULL, NULL))
        printk(KERN_ERR "SmartLamp: Erro ao encontrar endpoints.\n");
    
    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;
    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    // As linhas de leitura no probe foram removidas
    return 0;
}

// Executado quando o dispositivo USB é desconectado da USB
static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");
    kobject_put(sys_obj);      // Remove os arquivos em /sys/kernel/smartlamp
    kfree(usb_in_buffer);      // Desaloca buffers
    kfree(usb_out_buffer);
}


static void usb_send_cmd(const char *cmd) {
    int ret, actual_size;

    // Coloca o comando no buffer de saída, adicionando uma quebra de linha
    snprintf(usb_out_buffer, usb_max_size, "%s\n", cmd);

    // Envia a mensagem pela porta de saída (usb_out)
    ret = usb_bulk_msg(smartlamp_device, usb_sndbulkpipe(smartlamp_device, usb_out),
                       usb_out_buffer, strlen(usb_out_buffer), &actual_size, 1000);
    if (ret)
        printk(KERN_ERR "SmartLamp: Erro ao enviar comando '%s'.\n", cmd);
}

static int usb_read_serial() {
    int ret, actual_size;
    int retries = 10;
    static char response_buffer[MAX_RECV_LINE];
    int total_received = 0;

    memset(response_buffer, 0, sizeof(response_buffer));

    // Loop para ler a resposta da USB
    while (retries > 0) {
        ret = usb_bulk_msg(smartlamp_device, usb_rcvbulkpipe(smartlamp_device, usb_in),
                          usb_in_buffer, min(usb_max_size, MAX_RECV_LINE), &actual_size, 1500);

        if (ret) {
            retries--;
            continue;
        }

        if (total_received + actual_size < MAX_RECV_LINE) {
            memcpy(response_buffer + total_received, usb_in_buffer, actual_size);
            total_received += actual_size;
            response_buffer[total_received] = '\0';
        }

        if (strchr(response_buffer, '\n') || strchr(response_buffer, '\r')) {
            break; // Mensagem completa recebida
        }
    }

    // Processa a resposta recebida
    if (total_received > 0) {
        char cmd_str[20];
        int valor;

        // Tenta extrair da resposta o padrão "RES <COMANDO> <VALOR>"
        if (sscanf(response_buffer, "RES %19s %d", cmd_str, &valor) == 2) {
            printk(KERN_INFO "SmartLamp: Resposta do comando '%s' -> %d\n", cmd_str, valor);
            return valor; // Retorna o valor numérico extraído
        } else {
            printk(KERN_ERR "SmartLamp: Resposta invalida recebida: [%s]\n", response_buffer);
            return -EINVAL; // Erro de valor inválido
        }
    }

    return -ETIMEDOUT; // Erro de timeout se nada for lido
}

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é lido (e.g., cat /sys/kernel/smartlamp/led)
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff) {
    // value representa o valor do led ou ldr
    int value;
    // attr_name representa o nome do arquivo que está sendo lido (ldr ou led)
    const char *attr_name = attr->attr.name;
    char command[20];

    // printk indicando qual arquivo está sendo lido
    printk(KERN_INFO "SmartLamp: Lendo %s ...\n", attr_name);

    // Implemente a leitura do valor do led usando a função usb_read_serial()
    if (strcmp(attr_name, "led") == 0) {
        strcpy(command, "GET_LED");
    } else if (strcmp(attr_name, "ldr") == 0) {
        strcpy(command, "GET_LDR");
    } else {
        return -EINVAL;
    }

    usb_send_cmd(command);
    msleep(50);
    value = usb_read_serial();

    if (value < 0) {
        return value;
    }

    sprintf(buff, "%d\n", value);                   // Cria a mensagem com o valor do led, ldr
    return strlen(buff);
}

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é escrito (e.g., echo "100" | sudo tee -a /sys/kernel/smartlamp/led)
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count) {
    long ret, value;
    const char *attr_name = attr->attr.name;

    // Converte o valor recebido para long
    ret = kstrtol(buff, 10, &value);
    if (ret) {
        printk(KERN_ALERT "SmartLamp: valor de %s invalido.\n", attr_name);
        return -EACCES;
    }

    printk(KERN_INFO "SmartLamp: Setando %s para %ld ...\n", attr_name, value);
    


    if (ret < 0) {
        printk(KERN_ALERT "SmartLamp: erro ao setar o valor do %s.\n", attr_name);
        return -EACCES;
    }

    return strlen(buff);
}