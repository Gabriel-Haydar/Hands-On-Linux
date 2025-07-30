#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102");
MODULE_LICENSE("GPL");


#define MAX_RECV_LINE 100 // Tamanho máximo de uma linha de resposta do dispositvo USB


static char recv_line[MAX_RECV_LINE];              // Armazena dados vindos da USB até receber um caractere de nova linha '\n'
static int recv_size = 0;                          // Quantidade de caracteres no recv_line
static struct usb_device *smartlamp_device;        // Referência para o dispositivo USB
static uint usb_in, usb_out;                       // Endereços das portas de entrada e saida da USB
static char *usb_in_buffer, *usb_out_buffer;       // Buffers de entrada e saída da USB
static int usb_max_size;                           // Tamanho máximo de uma mensagem USB

// OBS: Substitua os valores abaixo pelos IDs corretos do seu dispositivo.
// Estes são valores comuns para o chip CP2102.
#define VENDOR_ID   0x10C4 /* Encontre o VendorID  do smartlamp */
#define PRODUCT_ID  0xEA60 /* Encontre o ProductID do smartlamp */
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id); // Executado quando o dispositivo é conectado na USB
static void usb_disconnect(struct usb_interface *ifce);                           // Executado quando o dispositivo USB é desconectado da USB
static int  usb_send_cmd(char *cmd, int param);

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é lido (e.g., cat /sys/kernel/smartlamp/led)
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff);
// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é escrito (e.g., echo "100" | sudo tee -a /sys/kernel/smartlamp/led)
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count);
// Variáveis para criar os arquivos no /sys/kernel/smartlamp/{led, ldr}
static struct kobj_attribute  led_attribute = __ATTR(led, S_IRUGO | S_IWUSR, attr_show, attr_store);
static struct kobj_attribute  ldr_attribute = __ATTR(ldr, S_IRUGO | S_IWUSR, attr_show, attr_store); // LDR é apenas leitura (S_IRUGO)
static struct attribute      *attrs[]       = { &led_attribute.attr, &ldr_attribute.attr, NULL };
static struct attribute_group attr_group    = { .attrs = attrs };
static struct kobject        *sys_obj;

MODULE_DEVICE_TABLE(usb, id_table);

bool ignore = true;

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
    ignore = sysfs_create_group(sys_obj, &attr_group);

    // Detecta portas e aloca buffers de entrada e saída de dados na USB
    smartlamp_device = interface_to_usbdev(interface);
    ignore =  usb_find_common_endpoints(interface->cur_altsetting, &usb_endpoint_in, &usb_endpoint_out, NULL, NULL);
    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;
    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    return 0;
}

// Executado quando o dispositivo USB é desconectado da USB
static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");
    if (sys_obj) kobject_put(sys_obj);
    kfree(usb_in_buffer);
    kfree(usb_out_buffer);
}

// Envia um comando via USB, espera e retorna a resposta do dispositivo (convertido para int)
static int usb_send_cmd(char *cmd, int param) {
    int ret, actual_size, i;
    int retries = 10;
    char resp_expected[MAX_RECV_LINE];
    char *resp_pos;
    long resp_number = -1;

    printk(KERN_INFO "SmartLamp: Enviando comando: %s %d\n", cmd, param);

    if (param >= 0) {
        sprintf(usb_out_buffer, "%s %d\n", cmd, param);
    } else { // Caso não haja parâmetro, envia apenas o comando
        sprintf(usb_out_buffer, "%s\n", cmd);
    }

    ret = usb_bulk_msg(smartlamp_device, usb_sndbulkpipe(smartlamp_device, usb_out), usb_out_buffer, strlen(usb_out_buffer), &actual_size, 1000);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro de codigo %d ao enviar comando!\n", ret);
        return -1;
    }

    sprintf(resp_expected, "RES %s", cmd);

    while (retries > 0) {
        ret = usb_bulk_msg(smartlamp_device, usb_rcvbulkpipe(smartlamp_device, usb_in), usb_in_buffer, min(usb_max_size, MAX_RECV_LINE), &actual_size, 1000);
        if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao ler dados da USB (tentativa %d). Codigo: %d\n", retries--, ret);
            continue;
        }

        for (i = 0; i < actual_size; i++) {
            if (usb_in_buffer[i] == '\n') {
                recv_line[recv_size] = '\0';
                printk(KERN_INFO "SmartLamp: Recebido: '%s'\n", recv_line);

                if (!strncmp(recv_line, resp_expected, strlen(resp_expected))) {
                    resp_pos = strstr(recv_line, resp_expected);
                    sscanf(resp_pos, "%*s %*s %ld", &resp_number);
                    recv_size = 0; // Limpa o buffer para a próxima leitura
                    return resp_number;
                }
                recv_size = 0; // Limpa o buffer se a resposta não for a esperada
            } else {
                if (recv_size < MAX_RECV_LINE -1) {
                    recv_line[recv_size++] = usb_in_buffer[i];
                }
            }
        }
    }
    return -1;
}

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é lido
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff) {
    int value;
    const char *attr_name = attr->attr.name;

    printk(KERN_INFO "SmartLamp: Lendo %s ...\n", attr_name);

    if (strcmp(attr_name, "led") == 0) {
        value = usb_send_cmd("GET_LED", -1); // -1 indica que não há parâmetro
    } else { // ldr
        value = usb_send_cmd("GET_LDR", -1); // -1 indica que não há parâmetro
    }

    if (value < 0) {
        return -EACCES;
    }

    sprintf(buff, "%d\n", value);
    return strlen(buff);
}

// Executado quando o arquivo /sys/kernel/smartlamp/{led, ldr} é escrito
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count) {
    long ret, value;
    const char *attr_name = attr->attr.name;

    ret = kstrtol(buff, 10, &value);
    if (ret) {
        printk(KERN_ALERT "SmartLamp: valor de %s invalido.\n", attr_name);
        return -EACCES;
    }

    printk(KERN_INFO "SmartLamp: Setando %s para %ld ...\n", attr_name, value);

    //Utiliza usb_send_cmd para enviar o comando SET_LED
    if (strcmp(attr_name, "led") == 0) {
        ret = usb_send_cmd("SET_LED", value);
    } else {
        printk(KERN_ALERT "SmartLamp: O atributo %s eh somente leitura.\n", attr_name);
        return -EACCES;
    }

    if (ret < 0) {
        printk(KERN_ALERT "SmartLamp: erro ao setar o valor do %s.\n", attr_name);
        return -EACCES;
    }

    return count; // Retorna count em vez de strlen(buff) para escritas
}