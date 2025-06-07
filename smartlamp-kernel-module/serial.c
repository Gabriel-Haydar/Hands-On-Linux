#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/string.h> 
#include <linux/delay.h>    
#include <linux/kernel.h> 

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102)");
MODULE_LICENSE("GPL");

#define MAX_RECV_LINE 100

static struct usb_device *smartlamp_device;
static uint usb_in, usb_out;
static char *usb_in_buffer, *usb_out_buffer;
static int usb_max_size;

#define VENDOR_ID    0x0000  // Valor temporário
#define PRODUCT_ID   0x0000  // Valor temporário
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id);
static void usb_disconnect(struct usb_interface *interface);
static int usb_read_serial(void);

MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver smartlamp_driver = {
    .name       = "smartlamp",
    .probe      = usb_probe,
    .disconnect = usb_disconnect,
    .id_table   = id_table,
};

module_usb_driver(smartlamp_driver);

static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    struct usb_endpoint_descriptor *usb_endpoint_in = NULL, *usb_endpoint_out = NULL;
    int retval;
    int ldr_val;

    printk(KERN_INFO "SmartLamp: Dispositivo conectado ...\n");

    smartlamp_device = interface_to_usbdev(interface);

    retval = usb_find_common_endpoints(interface->cur_altsetting,
                                       &usb_endpoint_in, &usb_endpoint_out,
                                       NULL, NULL);
    if (retval) {
        printk(KERN_ERR "SmartLamp: Erro ao encontrar endpoints USB\n");
        return retval;
    }

    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;

    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    if (!usb_in_buffer || !usb_out_buffer) {
        printk(KERN_ERR "SmartLamp: Falha ao alocar buffers\n");
        kfree(usb_in_buffer);
        kfree(usb_out_buffer);
        return -ENOMEM;
    }

    ldr_val = usb_read_serial();
    printk(KERN_INFO "SmartLamp: LDR Value: %d\n", ldr_val);

    return 0;
}

static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");

    kfree(usb_in_buffer);
    kfree(usb_out_buffer);
}

static int usb_read_serial(void) {
    int ret, actual_size;
    int retries = 10;

    while (retries-- > 0) {
        memset(usb_in_buffer, 0, usb_max_size);

        ret = usb_bulk_msg(smartlamp_device,
                           usb_rcvbulkpipe(smartlamp_device, usb_in),
                           usb_in_buffer,
                           min(usb_max_size, MAX_RECV_LINE),
                           &actual_size,
                           1000);
        if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao ler dados da USB (tentativa %d). Código: %d\n", retries, ret);
            msleep(100);
            continue;
        }

        if (strncmp(usb_in_buffer, "RES_LDR ", 8) == 0) {
            long val;
            if (kstrtol(usb_in_buffer + 8, 10, &val) == 0)
                return (int)val;
        }
    }

    printk(KERN_ERR "SmartLamp: Timeout na leitura da USB\n");
    return -EIO;
}
