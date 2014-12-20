/*
 * hdElastos.h
 */

#ifndef __hdElastosMantle_H__
#define __hdElastosMantle_H__

#include <elatypes.h>

/* RT-Thread error code definitions */
#define RT_EOK                          0               /**< There is no error */
#define RT_ERROR                        1               /**< A generic error happens */
#define RT_ETIMEOUT                     2               /**< Timed out */
#define RT_EFULL                        3               /**< The resource is full */
#define RT_EEMPTY                       4               /**< The resource is empty */
#define RT_ENOMEM                       5               /**< No memory */
#define RT_ENOSYS                       6               /**< No system */
#define RT_EBUSY                        7               /**< Busy */
#define RT_EIO                          8               /**< IO error */

#define RT_NAME_MAX     6
struct rt_list_node
{
    struct rt_list_node *next;                          /**< point to next node. */
    struct rt_list_node *prev;                          /**< point to prev node. */
};
typedef struct rt_list_node rt_list_t;                  /**< Type for lists. */


/**
 * Base structure of Kernel object
 */
struct rt_object
{
    char       name[RT_NAME_MAX];                       /**< name of kernel object */
    UInt8      type;                                    /**< type of kernel object */
    UInt8      flag;                                    /**< flag of kernel object */
    rt_list_t  list;                                    /**< list node of kernel object */
};
typedef struct rt_object *rt_object_t;                  /**< Type for kernel objects. */

/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN(size, align)
 * Return the most contiguous size aligned at specified width. RT_ALIGN(13, 4)
 * would return 16.
 */
#define RT_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))

/**
 * @ingroup BasicDef
 *
 * @def RT_ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. RT_ALIGN_DOWN(13, 4)
 * would return 12.
 */
#define RT_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))


/**
 * device (I/O) class type
 */
enum rt_device_class_type
{
    RT_Device_Class_Char = 0,                           /**< character device */
    RT_Device_Class_Block,                              /**< block device */
    RT_Device_Class_NetIf,                              /**< net interface */
    RT_Device_Class_MTD,                                /**< memory device */
    RT_Device_Class_CAN,                                /**< CAN device */
    RT_Device_Class_RTC,                                /**< RTC device */
    RT_Device_Class_Sound,                              /**< Sound device */
    RT_Device_Class_Graphic,                            /**< Graphic device */
    RT_Device_Class_I2CBUS,                             /**< I2C bus device */
    RT_Device_Class_USBDevice,                          /**< USB slave device */
    RT_Device_Class_USBHost,                            /**< USB host bus */
    RT_Device_Class_SPIBUS,                             /**< SPI bus device */
    RT_Device_Class_SPIDevice,                          /**< SPI device */
    RT_Device_Class_SDIO,                               /**< SDIO bus device */
    RT_Device_Class_PM,                                 /**< PM pseudo device */
    RT_Device_Class_Pipe,                               /**< Pipe device */
    RT_Device_Class_Portal,                             /**< Portal device */
    RT_Device_Class_Miscellaneous,                      /**< Miscellaneous device */
    RT_Device_Class_Unknown                             /**< unknown device */
};

/**
 * device flags defitions
 */
#define RT_DEVICE_FLAG_DEACTIVATE       0x000           /**< device is not not initialized */

#define RT_DEVICE_FLAG_RDONLY           0x001           /**< read only */
#define RT_DEVICE_FLAG_WRONLY           0x002           /**< write only */
#define RT_DEVICE_FLAG_RDWR             0x003           /**< read and write */

#define RT_DEVICE_FLAG_REMOVABLE        0x004           /**< removable device */
#define RT_DEVICE_FLAG_STANDALONE       0x008           /**< standalone device */
#define RT_DEVICE_FLAG_ACTIVATED        0x010           /**< device is activated */
#define RT_DEVICE_FLAG_SUSPENDED        0x020           /**< device is suspended */
#define RT_DEVICE_FLAG_STREAM           0x040           /**< stream mode */

#define RT_DEVICE_FLAG_INT_RX           0x100           /**< INT mode on Rx */
#define RT_DEVICE_FLAG_DMA_RX           0x200           /**< DMA mode on Rx */
#define RT_DEVICE_FLAG_INT_TX           0x400           /**< INT mode on Tx */
#define RT_DEVICE_FLAG_DMA_TX           0x800           /**< DMA mode on Tx */

#define RT_DEVICE_OFLAG_CLOSE           0x000           /**< device is closed */
#define RT_DEVICE_OFLAG_RDONLY          0x001           /**< read only access */
#define RT_DEVICE_OFLAG_WRONLY          0x002           /**< write only access */
#define RT_DEVICE_OFLAG_RDWR            0x003           /**< read and write */
#define RT_DEVICE_OFLAG_OPEN            0x008           /**< device is opened */

/**
 * general device commands
 */
#define RT_DEVICE_CTRL_RESUME           0x01            /**< resume device */
#define RT_DEVICE_CTRL_SUSPEND          0x02            /**< suspend device */

/**
 * special device commands
 */
#define RT_DEVICE_CTRL_CHAR_STREAM      0x10            /**< stream mode on char device */
#define RT_DEVICE_CTRL_BLK_GETGEOME     0x10            /**< get geometry information   */
#define RT_DEVICE_CTRL_BLK_SYNC         0x11            /**< flush data to block device */
#define RT_DEVICE_CTRL_BLK_ERASE        0x12            /**< erase block on block device */
#define RT_DEVICE_CTRL_BLK_AUTOREFRESH  0x13            /**< block device : enter/exit auto refresh mode */
#define RT_DEVICE_CTRL_NETIF_GETMAC     0x10            /**< get mac address */
#define RT_DEVICE_CTRL_MTD_FORMAT       0x10            /**< format a MTD device */
#define RT_DEVICE_CTRL_RTC_GET_TIME     0x10            /**< get time */
#define RT_DEVICE_CTRL_RTC_SET_TIME     0x11            /**< set time */
#define RT_DEVICE_CTRL_RTC_GET_ALARM    0x12            /**< get alarm */
#define RT_DEVICE_CTRL_RTC_SET_ALARM    0x13            /**< set alarm */

typedef struct rt_device *rt_device_t;
/**
 * Device structure
 */
struct rt_device
{
    struct rt_object          parent;                   /**< inherit from rt_object */

    enum rt_device_class_type type;                     /**< device type */
    UInt16                    flag;                     /**< device flag */
    UInt16                    open_flag;                /**< device open flag */

    UInt8                     ref_count;                /**< reference count */
    UInt8                     device_id;                /**< 0 - 255 */

    /* device call back */
    Int32 (*rx_indicate)(rt_device_t dev, UInt32 size);
    Int32 (*tx_complete)(rt_device_t dev, void *buffer);

    /* common device interface */
    Int32  (*init)   (rt_device_t dev);
    Int32  (*open)   (rt_device_t dev, UInt16 oflag);
    Int32  (*close)  (rt_device_t dev);
    UInt32 (*read)   (rt_device_t dev, Int32 pos, void *buffer, UInt32 size);
    UInt32 (*write)  (rt_device_t dev, Int32 pos, const void *buffer, UInt32 size);
    Int32  (*control)(rt_device_t dev, UInt8 cmd, void *args);

    void   *user_data;                /**< device private data */
};

/**
 * block device geometry structure
 */
struct rt_device_blk_geometry
{
    UInt32 sector_count;                           /**< count of sectors */
    UInt32 bytes_per_sector;                       /**< number of bytes per sector */
    UInt32 block_size;                             /**< number of bytes to erase one block */
};

/**
 * sector arrange struct on block device
 */
struct rt_device_blk_sectors
{
    UInt32 sector_begin;                           /**< begin sector */
    UInt32 sector_end;                             /**< end sector   */
};


/*@}*/
#endif
