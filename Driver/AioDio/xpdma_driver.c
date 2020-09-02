#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/time.h>
#include "xpdma_driver.h"
#include <linux/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("PCIe driver for XDMA, Linux");
MODULE_AUTHOR("wangyong");

#define BUF_SIZE  (4<<20)      // 4 MBytes read/write buffer size

///节点
enum CDEV_TYPE {
    CHAR_IN = 0,
    CHAR_OUT = 1
};

typedef union _LARGE_INTEGER   {   
	struct {
        uint32_t LowPart;   
        uint32_t HighPart;   
	};
    uint64_t QuadPart;
}   LARGE_INTEGER;

typedef struct _BarMap {
    unsigned long hdwr_addr;
    unsigned long length;
    void *virt_addr;
    int busy;
} BarMap;

// XDMA descriptor
typedef struct _DMA_DESCRIPTOR {
    uint32_t control;
    uint32_t numBytes;  // transfer length in bytes
    uint32_t srcAddrLo; // source address (low 32-bit)
    uint32_t srcAddrHi; // source address (high 32-bit)
    uint32_t dstAddrLo; // destination address (low 32-bit)
    uint32_t dstAddrHi; // destination address (high 32-bit)
                        // next descriptor in the single-linked list of descriptors, 
                        // this is the bus address of the next descriptor in the root complex memory.
    uint32_t nextLo;    // next desc address (low 32-bit)
    uint32_t nextHi;    // next desc address (high 32-bit)
} DMA_DESCRIPTOR;

struct _XdmaPciDev;

typedef struct _XdmaCharDev {
    long busy;
    int valid;
    struct _XdmaPciDev *xpdev;
    struct cdev cdev;
    dev_t cdevno;
    struct device *sys_device; 
} XdmaCharDev;

typedef struct _XdmaPciDev {
    struct pci_dev *pdev;
    struct pci_device_id pci_id;
    int major;
    XdmaCharDev xcdev[2];
    
    const char *boardName;
    int slot;
    BarMap bar[2];
    DMA_DESCRIPTOR *descChain;
    char *buf;
    size_t descChainLength;
    dma_addr_t descChainHWAddr;
    dma_addr_t bufHWAddr;
} XdmaPciDev;

typedef struct _BoardName {
    uint32_t vendor;
    uint32_t device;
    const char *name;
} BoardName;

static struct class *g_xdmaClass = NULL;   // Global variable for the device class
static BoardName g_boardName[] = {
    {0x10ee, 0x7021, "xdma"},
    {0x10ee, 0x7022, "xdmb"},
    {0x10ee, 0x2010, "dio"},
    {0x10ee, 0x2020, "aio"},
    {0x10ee, 0x2030, "audio"},
    {0, 0, NULL}
};

//节点名称
static const char * const  g_devnodeNames[] = {
    "%s_%d_in",
    "%s_%d_out"
};

static const struct pci_device_id pciIds[] = {
    { PCI_DEVICE(0x10ee, 0x7021), },
    { PCI_DEVICE(0x10ee, 0x7022), },
    { PCI_DEVICE(0x10ee, 0x2010), },    ///DIO
    { PCI_DEVICE(0x10ee, 0x2020), },    ///AIO
//    { PCI_DEVICE(0x10ee, 0x2030), },    ///AUDIO
	{0,}
};
MODULE_DEVICE_TABLE(pci, pciIds);

static long XdmaDmaIoctl (struct file *filp, unsigned int cmd, unsigned long arg);
static int XdmaDmaOpen(struct inode *inode, struct file *filp);
static int XdmaDmaRelease(struct inode *inode, struct file *filp);
static int XdmaDmaMmap(struct file *file, struct vm_area_struct *vma);

static int XdmaUserOpen(struct inode *inode, struct file *filp);
static int XdmaUserRelease(struct inode *inode, struct file *filp);
static int XdmaUserMmap(struct file *file, struct vm_area_struct *vma);

static ssize_t XdmaSendData(XdmaPciDev *xpdev, size_t count, u32 addr);
static ssize_t XdmaRecvData(XdmaPciDev *xpdev, size_t count, u32 addr);

static long XdmaInOutIoctl (struct file *filp, unsigned int cmd, unsigned long arg);
static int XdmaInOutOpen(struct inode *inode, struct file *filp);
static int XdmaInOutRelease(struct inode *inode, struct file *filp);
static int XdmaInOutMmap(struct file *file, struct vm_area_struct *vma);

///DMA OPER
static struct file_operations g_xdmaDmaOps = {
    .unlocked_ioctl = XdmaDmaIoctl,
    .open           = XdmaDmaOpen,
    .release        = XdmaDmaRelease,
    .mmap           = XdmaDmaMmap
};
///USER OPER
static struct file_operations g_xdmaUserOps = {
    .open           = XdmaUserOpen,
    .release        = XdmaUserRelease,
    .mmap           = XdmaUserMmap
};
///IN OUT OPER
static struct file_operations g_xdmaInOutOps = {
    .open           = XdmaInOutOpen,
    .release        = XdmaInOutRelease,
    .mmap           = XdmaInOutMmap,
    .unlocked_ioctl = XdmaInOutIoctl
};

static inline u32 XdmaReadReg(XdmaPciDev *xpdev, u32 reg)
{
    return readl(xpdev->bar[0].virt_addr + reg);
}

static inline void XdmaWriteReg(XdmaPciDev *xpdev, u32 reg, u32 val)
{
    writel(val, (xpdev->bar[0].virt_addr + reg));
}

static inline u32 XdmaDmaReadReg(XdmaPciDev *xpdev, u32 reg)
{
    return readl(xpdev->bar[1].virt_addr + reg);
}

static inline void XdmaDmaWriteReg(XdmaPciDev *xpdev, u32 reg, u32 val)
{
    writel(val, (xpdev->bar[1].virt_addr + reg));
}

static inline u32 GetCfgReg32(XdmaPciDev *xpdev, u32 offset)
{
    return XdmaDmaReadReg(xpdev, offset);
}

static inline void SetCfgReg32(XdmaPciDev *xpdev, u32 offset, u32 data)
{
    XdmaDmaWriteReg(xpdev, offset, data);
}

static inline u32 GetChannelReg32(XdmaPciDev *xpdev, u32 offset, u32 channel)
{
    return XdmaDmaReadReg(xpdev, offset + 0x100 * channel);
}

static inline void SetChannelReg32(XdmaPciDev *xpdev, u32 offset, u32 channel, u32 data)
{
    return XdmaDmaWriteReg(xpdev, offset + 0x100 * channel, data);
}

static int XdmaDmaOpen(struct inode *inode, struct file *file)
{
    XdmaCharDev *xcdev = container_of(inode->i_cdev, struct _XdmaCharDev, cdev);
    XdmaPciDev *xpdev = xcdev->xpdev;
    
    if (test_and_set_bit(0, &xcdev->busy)) {
        printk(KERN_WARNING"device can't duplicate opened (%s,%X,%X)\n",
               xpdev->boardName, xpdev->pci_id.vendor, xpdev->pci_id.device);
        return -2;
    }
    
    printk(KERN_INFO"Open dma dev: module opened (%s,%X,%X)\n",
           xpdev->boardName, xpdev->pci_id.vendor, xpdev->pci_id.device);
        
    file->private_data = xpdev;
        
    return SUCCESS;
}

static int XdmaDmaRelease(struct inode *inode, struct file *file)
{
    XdmaCharDev *xcdev = container_of(inode->i_cdev, struct _XdmaCharDev, cdev);
    
    printk(KERN_INFO"%s: Release dma dev: module released\n", DEVICE_NAME);
    xcdev->busy = 0;
    
    return SUCCESS;
}

static int XdmaDmaMmap(struct file *file, struct vm_area_struct *vma)
{
    ///NOTE::映射问题
    XdmaPciDev *xpdev = file->private_data;
    
    printk(KERN_INFO"XdmaDmaMmap dma (%s,%X,%X)\n",
           xpdev->boardName, xpdev->pci_id.vendor, xpdev->pci_id.device);
    
    vma->vm_flags |= (VM_IO | VM_DONTEXPAND | VM_DONTDUMP);  //表示对设备IO空间的映射  
    if (remap_pfn_range(vma,            //虚拟内存区域，即设备地址将要映射到这里  
                       vma->vm_start,   //虚拟空间的起始地址          
                       xpdev->bufHWAddr >> 12,       //与物理内存对应的页帧号，物理地址右移12位 
                       vma->vm_end - vma->vm_start,     //映射区域大小，一般是页大小的整数倍  
                       vma->vm_page_prot))              //保护属性，  
    {  
        printk(KERN_WARNING"mmap dma error\n");
        return -EAGAIN;  
    }
    
    printk(KERN_INFO"mmap dma ok\n");
    return 0;
}

static long XdmaDmaIoctl (struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = CRIT_ERR;
    Buffer_t buf;
    XdmaPciDev *xpdev = (XdmaPciDev *)file->private_data;

    if (_IOC_TYPE(cmd) != IOC_MAGIC) {
        printk(KERN_WARNING"XdmaDmaIoctl ioc magic error 0x%X\n", cmd);
        return -1;
    }
    
    if (copy_from_user(&buf, (void *)arg, sizeof(Buffer_t)))
    {
        printk(KERN_WARNING"XdmaDmaIoctl copy from user failed\n");
    }
    
    printk(KERN_INFO"XdmaDmaIoctl cmd %X, (%s,%X,%X)\n", cmd,
           xpdev->boardName, xpdev->pci_id.vendor, xpdev->pci_id.device);
  
    switch (_IOC_DIR(cmd)) {
        case _IOC_WRITE:
            printk(KERN_INFO"ioctl_send (%d, %X)\n", buf.count, buf.addr); 
            ret = XdmaSendData(xpdev, buf.count, buf.addr);
            break;
            
        case _IOC_READ:
            printk(KERN_INFO"ioctl_recv (%d, %X)\n", buf.count, buf.addr); 
            ret = XdmaRecvData(xpdev, buf.count, buf.addr);
            break;

        default:
            break;
    }

    return ret;
}

static int XdmaUserOpen(struct inode *inode, struct file *file)
{
    XdmaCharDev *xcdev = container_of(inode->i_cdev, struct _XdmaCharDev, cdev);
    XdmaPciDev *xpdev = xcdev->xpdev;
    
    if (test_and_set_bit(0, &xcdev->busy)) {
        printk(KERN_INFO"device can't duplicate opened (%s,%X,%X)\n",
           xpdev->boardName, xpdev->pci_id.vendor, xpdev->pci_id.device);
        return -2;
    }
    
    printk(KERN_INFO"Open user dev: module opened (%s,%X,%X)\n",
           xpdev->boardName, xpdev->pci_id.vendor, xpdev->pci_id.device);
        
    file->private_data = xpdev;
        
    return SUCCESS;
}

static int XdmaUserRelease(struct inode *inode, struct file *file)
{
    XdmaCharDev *xcdev = container_of(inode->i_cdev, struct _XdmaCharDev, cdev);
    
    printk(KERN_INFO"%s: Release dma dev: module released\n", DEVICE_NAME);
    xcdev->busy = 0;
    
    return SUCCESS;
}

static int XdmaUserMmap(struct file *file, struct vm_area_struct *vma)
{
    ///NOTE::映射
    XdmaPciDev *xpdev = file->private_data;
    
    printk(KERN_INFO"XdmaUserMmap user (%s,%X,%X)\n",
           xpdev->boardName, xpdev->pci_id.vendor, xpdev->pci_id.device);
    
    vma->vm_flags |= (VM_IO | VM_DONTEXPAND | VM_DONTDUMP);  //表示对设备IO空间的映射  
    if (remap_pfn_range(vma,            //虚拟内存区域，即设备地址将要映射到这里  
                       vma->vm_start,   //虚拟空间的起始地址          
                       xpdev->bar[0].hdwr_addr >> 12,       //与物理内存对应的页帧号，物理地址右移12位 
                       vma->vm_end - vma->vm_start,     //映射区域大小，一般是页大小的整数倍  
                       vma->vm_page_prot))              //保护属性，  
    {  
        printk(KERN_WARNING"mmap user error\n");
        return -EAGAIN;  
    }
    
    printk(KERN_INFO"mmap user ok\n");
    return 0;
}

static int XdmaInOutOpen(struct inode *inode, struct file *file)
{
    XdmaCharDev *xcdev = container_of(inode->i_cdev, struct _XdmaCharDev, cdev);
    XdmaPciDev *xpdev = xcdev->xpdev;

    if (test_and_set_bit(0, &xcdev->busy)) {
        printk(KERN_INFO"device can't duplicate opened (%s,%X,%X)\n",
           xpdev->boardName, xpdev->pci_id.vendor, xpdev->pci_id.device);
        return -2;
    }

    printk(KERN_INFO"Open Output dev: module opened (%s,%X,%X)\n",
           xpdev->boardName, xpdev->pci_id.vendor, xpdev->pci_id.device);

    file->private_data = xpdev;

    return SUCCESS;
}

static int XdmaInOutRelease(struct inode *inode, struct file *file)
{
    XdmaCharDev *xcdev = container_of(inode->i_cdev, struct _XdmaCharDev, cdev);

    printk(KERN_INFO"%s: Release dma dev: module released\n", DEVICE_NAME);
    xcdev->busy = 0;

    return SUCCESS;
}

static int XdmaInOutMmap(struct file *file, struct vm_area_struct *vma)
{
    ///TODO::多设备映射，包括通用寄存器以及DMA寄存器
    XdmaPciDev *xpdev = file->private_data;
    int iLength = vma->vm_end - vma->vm_start;

    printk(KERN_INFO"XdmaOutputMmap output (%s,%X,%X)\n",
           xpdev->boardName, xpdev->pci_id.vendor, xpdev->pci_id.device);

    vma->vm_flags |= (VM_IO | VM_DONTEXPAND | VM_DONTDUMP);  //表示对设备IO空间的映射
    if (iLength <= 0x10000)
    {
        if (remap_pfn_range(vma,            //虚拟内存区域，即设备地址将要映射到这里
                            vma->vm_start,   //虚拟空间的起始地址
                            xpdev->bar[0].hdwr_addr >> 12,       //与物理内存对应的页帧号，物理地址右移12位
                            iLength,     //映射区域大小，一般是页大小的整数倍
                            vma->vm_page_prot))              //保护属性，
        {
            printk(KERN_WARNING"mmap output user left error\n");
            return -EAGAIN;
        }
    }
    else
    {
        if (remap_pfn_range(vma,
                            vma->vm_start,
                            xpdev->bar[0].hdwr_addr >> 12,
                            0x10000,
                            vma->vm_page_prot))
        {
            printk(KERN_WARNING"mmap output user right error\n");
            return -EAGAIN;
        }
        if (remap_pfn_range(vma,
                           vma->vm_start + 0x10000,
                           xpdev->bufHWAddr >> 12,
                           iLength - 0x10000,
                           vma->vm_page_prot))
        {
            printk(KERN_WARNING"mmap output dma right error\n");
            return -EAGAIN;
        }
    }
    printk(KERN_INFO"mmap user ok\n");
    return 0;
}

static long XdmaInOutIoctl (struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = CRIT_ERR;
    Buffer_t buf;
    XdmaPciDev *xpdev = (XdmaPciDev *)file->private_data;

    if (_IOC_TYPE(cmd) != IOC_MAGIC) {
        printk(KERN_WARNING"XdmaDmaIoctl ioc magic error 0x%X\n", cmd);
        return -1;
    }

    if (copy_from_user(&buf, (void *)arg, sizeof(Buffer_t)))
    {
        printk(KERN_WARNING"XdmaDmaIoctl copy from user failed\n");
    }

    printk(KERN_INFO"XdmaDmaIoctl cmd %X, (%s,%X,%X)\n", cmd,
           xpdev->boardName, xpdev->pci_id.vendor, xpdev->pci_id.device);

    switch (_IOC_DIR(cmd)) {
        case _IOC_WRITE:
            printk(KERN_INFO"ioctl_send (%d, %X)\n", buf.count, buf.addr);
            ret = XdmaSendData(xpdev, buf.count, buf.addr);
            break;

        case _IOC_READ:
            printk(KERN_INFO"ioctl_recv (%d, %X)\n", buf.count, buf.addr);
            ret = XdmaRecvData(xpdev, buf.count, buf.addr);
            break;

        default:
            break;
    }

    return ret;
}

static u32 ConfigureDescriptor(XdmaPciDev *xpdev, DMA_DESCRIPTOR *descriptor, u32 length, u32 offset, bool toDevice)
{
    LARGE_INTEGER srcAddr;
    LARGE_INTEGER dstAddr;
    LARGE_INTEGER desAddr;
    LARGE_INTEGER nextDesAddr;
    u32 bytesCount = 0;
    u32 channel = 0;
    u32 descriptorCount = 0;
    u32 i = 0;

    desAddr.QuadPart = xpdev->descChainHWAddr;
    nextDesAddr.QuadPart = desAddr.QuadPart + sizeof(DMA_DESCRIPTOR);

    for (i = 0; i < 1; i++)
    {
        descriptorCount++;
        descriptor->control = XDMA_DESC_MAGIC;
        descriptor->numBytes = min((length - bytesCount), (u32)BUF_SIZE);

        if (toDevice)
        {
            //srcAddr.QuadPart = xpdmas[i].writeHWAddr;
            srcAddr.QuadPart = xpdev->bufHWAddr;
            dstAddr.QuadPart = offset + bytesCount;

            descriptor->srcAddrHi = srcAddr.HighPart;
            descriptor->srcAddrLo = srcAddr.LowPart;
            descriptor->dstAddrHi = dstAddr.HighPart;
            descriptor->dstAddrLo = dstAddr.LowPart;
        }
        else
        {
            srcAddr.QuadPart = offset + bytesCount;
            dstAddr.QuadPart = xpdev->bufHWAddr;
            //dstAddr.QuadPart = xpdmas[i].readHWAddr;

            descriptor->srcAddrHi = srcAddr.HighPart;
            descriptor->srcAddrLo = srcAddr.LowPart;
            descriptor->dstAddrHi = dstAddr.HighPart;
            descriptor->dstAddrLo = dstAddr.LowPart;
        }

        bytesCount += descriptor->numBytes;

        // next descriptor address
        if (bytesCount < length)
        {
            descriptor->nextHi = nextDesAddr.HighPart;
            descriptor->nextLo = nextDesAddr.LowPart;
        }
        else
        {
            descriptor->control |= XDMA_DESC_STOP_BIT | XDMA_DESC_EOP_BIT | XDMA_DESC_COMPLETED_BIT;
            descriptor->nextHi = 0x0;
            descriptor->nextLo = 0x0;
            break;
        }

        //page++;
        descriptor++;
        nextDesAddr.QuadPart += sizeof(DMA_DESCRIPTOR);
    }

    // write descriptor address
    SetChannelReg32(xpdev, toDevice ? XDMA_H2C_SGDMA_DESC_LOW : XDMA_C2H_SGDMA_DESC_LOW, channel, desAddr.LowPart);
    SetChannelReg32(xpdev, toDevice ? XDMA_H2C_SGDMA_DESC_HIGH : XDMA_C2H_SGDMA_DESC_HIGH, channel, desAddr.HighPart);
    SetChannelReg32(xpdev, toDevice ? XDMA_H2C_SGDMA_DESC_ADJACENT : XDMA_C2H_SGDMA_DESC_ADJACENT, channel, 0);

    return descriptorCount;
}


static void StartEngine(XdmaPciDev *xpdev, bool toDevice)
{
    u32 status = 0;
    u32 ctlCode = 0xFFFFFF;
    u32 channel = 0;

    // Dummy read to flush pending writes
    status = GetCfgReg32(xpdev, XDMA_IRQ_BLK_CHNL_INT_REQUEST);

    // Dummy read to flush all previous writes
    status = GetChannelReg32(xpdev, toDevice ? XDMA_H2C_CHNL_STATUS_RC : XDMA_C2H_CHNL_STATUS_RC, channel);

    SetChannelReg32(xpdev, toDevice ? XDMA_H2C_CHNL_CTRL : XDMA_C2H_CHNL_CTRL, channel, ctlCode);
}

static void StopEngine(XdmaPciDev *xpdev, bool toDevice)
{
    u32 status = 0;
    u32 channel = 0;

    // STOP DMA
    SetChannelReg32(xpdev, toDevice ? XDMA_H2C_CHNL_CTRL_W1C : XDMA_C2H_CHNL_CTRL_W1C, channel, XDMA_CTRL_RUN_STOP);

    // Dummy read to flush all previous writes
    status = GetChannelReg32(xpdev, toDevice ? XDMA_H2C_CHNL_STATUS : XDMA_C2H_CHNL_STATUS, channel);

    // Dummy read to flush pending writes
    status = GetCfgReg32(xpdev, XDMA_IRQ_BLK_CHNL_INT_REQUEST);
}

static u32 DmaPollCompletion(XdmaPciDev *xpdev, u32 descriptorCount, bool toDevice)
{
    u32 channel = 0;
    u32 finishCount = 0;
    u32 status;
    u32 delayTime = 1000;

    do {
        status = GetChannelReg32(xpdev, toDevice ? XDMA_H2C_CHNL_STATUS_RC : XDMA_C2H_CHNL_STATUS_RC, channel);
        finishCount = GetChannelReg32(xpdev, toDevice ? XDMA_H2C_CHNL_COMPLETED_DESC_COUNT : XDMA_C2H_CHNL_COMPLETED_DESC_COUNT, channel);
        
        if (status & XDMA_STAT_IDLE_STOPPED)
        {
            return -1;
        }

        if (status & ~(XDMA_STAT_BUSY | XDMA_STAT_DESC_STOPPED | XDMA_STAT_DESC_COMPLETED))
        {
            return -1;
        }

        if (--delayTime == 0)
        {   
            return -2;
        }

        msleep(1);

    } while (finishCount < descriptorCount);

    return 0;
}

static int DmaRead(XdmaPciDev *xpdev, u32 length, u32 offset, u32 *actualTransBytes)
{    
    u32 result = 0;
    u32 descriptorCount = 0;

    StopEngine(xpdev, false);
    descriptorCount = ConfigureDescriptor(xpdev, xpdev->descChain, length, offset, false);

    StartEngine(xpdev, false);

    result = DmaPollCompletion(xpdev, descriptorCount, false);

    *actualTransBytes = (0 == result) ? length : 0;

    StopEngine(xpdev, false);

    return result;
}

static int DmaWrite(XdmaPciDev *xpdev, u32 length, u32 offset, u32 *actualTransBytes)
{
    u32 result = 0;
    u32 descriptorCount = 0;

    StopEngine(xpdev, true);
    descriptorCount = ConfigureDescriptor(xpdev, xpdev->descChain, length, offset, true);

    StartEngine(xpdev, true);
    
    result = DmaPollCompletion(xpdev, descriptorCount, true);

    *actualTransBytes = (0 == result) ? length : 0;

    StopEngine(xpdev, true);

    return result;
}

static int SgBlock(XdmaPciDev *xpdev, int direction, size_t count, u32 addr)
{
    size_t unsended = count;
    u32 curAddr = addr;
    u32 btt = BUF_SIZE;
    u32 actualTransBytes = 0;
    u32 result = 0;

    if ((addr % 4) != 0 )  {
        printk(KERN_WARNING"%s: Scatter Gather: Address %08X not dword aligned.\n", DEVICE_NAME, addr);
        return (CRIT_ERR);
    }

    while (unsended) {
        btt = (unsended < BUF_SIZE) ? unsended : BUF_SIZE;
        printk(KERN_INFO"%s: SG Block: BTT=%u\tunsended=%lu \n", DEVICE_NAME, btt, unsended);

        if (PCI_DMA_TODEVICE == direction) {
            result = DmaWrite(xpdev, btt, curAddr, &actualTransBytes);
            printk(KERN_INFO"dma write %d bytes to 0x%X, res %d, acutual write %d bytes\n",
                   btt, curAddr, result, actualTransBytes);
        }
        
        if (PCI_DMA_FROMDEVICE == direction) {
            result = DmaRead(xpdev, btt, curAddr, &actualTransBytes);
            printk(KERN_INFO"dma read %d bytes from 0x%X, res %d, acutual read %d bytes\n",
                   btt, curAddr, result, actualTransBytes);
        }

        curAddr += BUF_SIZE;
        unsended -= btt;
    }

    return SUCCESS;
}

static ssize_t XdmaSendData(XdmaPciDev *xpdev, size_t count, u32 addr)
{
    if (count > BUF_SIZE) {
        printk(KERN_WARNING"send size error %ld\n", count);
        return CRIT_ERR;
    }

    SgBlock(xpdev, PCI_DMA_TODEVICE, count, addr);

    return SUCCESS;
}

static ssize_t XdmaRecvData(XdmaPciDev *xpdev, size_t count, u32 addr)
{
    if (count > BUF_SIZE) {
        printk(KERN_WARNING"recv size error %ld\n", count);
        return CRIT_ERR;
    }

    SgBlock(xpdev, PCI_DMA_FROMDEVICE, count, addr);

    return SUCCESS;
}

static int MapSingleBar(XdmaPciDev *xpdev, int barId)
{
    int ret = 0;
    BarMap *map = &xpdev->bar[barId];
    
    map->hdwr_addr = pci_resource_start(xpdev->pdev, barId);
    if (0 > map->hdwr_addr) {
        printk(KERN_WARNING"%s: getResource: BAR%d Address not set.\n", DEVICE_NAME, barId);
        return CRIT_ERR;
    }
    printk(KERN_INFO"BAR%d hdwr addr = 0x%lX\n", barId, map->hdwr_addr);

    // Get the Base Address Length
    map->length = pci_resource_len(xpdev->pdev, barId);
    printk(KERN_INFO"BAR%d length = 0x%lX\n", barId, map->length);

    // Get Virtual HW address
    map->virt_addr = ioremap(map->hdwr_addr, map->length);
    if (!map->virt_addr) {
        printk(KERN_WARNING"%s: getResource: Could not remap BAR%d memory.\n", DEVICE_NAME, barId);
        return CRIT_ERR;
    }

    printk(KERN_INFO"BAR%d virt addr = %p\n", barId, map->virt_addr);

    // NOTE::Obsolete 4.15
    // Check the memory region to see if it is in use
//    if (check_mem_region(map->hdwr_addr, map->length)) {
//        printk(KERN_WARNING"%s: getResource: BAR%d Memory in use.\n", DEVICE_NAME, barId);
//        ret = CRIT_ERR;
//        goto unmap;
//    }

    // Try to gain exclusive control of memory for hardware.
    if (NULL == request_mem_region(map->hdwr_addr, map->length, "Xilinx_PCIe_CDMA_Driver")) {
        printk(KERN_WARNING"%s: getResource: BAR%d Memory in use.\n", DEVICE_NAME, barId);
        ret = CRIT_ERR;
        goto unmap;
    }
    map->busy = 1;
    return 0;
    
unmap :
    iounmap(map->virt_addr);
    return ret;
}

static void UnmapSingleBar(XdmaPciDev *xpdev, int barId)
{
    BarMap *map = &xpdev->bar[barId];
    
    if (NULL != map->virt_addr) {
        iounmap(map->virt_addr);
        map->virt_addr = NULL;
    }
    
    if (map->busy) {
        release_mem_region(map->hdwr_addr, map->length);
        map->busy = 0;
    }
}

static int XdmaGetResource(XdmaPciDev *xpdev) 
{
    int ret = 0;

    // Set Bus Master Enable (BME) bit
    pci_set_master(xpdev->pdev);
    
    ret = MapSingleBar(xpdev, 0);
    if (ret) {
        return ret;
    }
    
    ret = MapSingleBar(xpdev, 1);
    if (ret) {
        UnmapSingleBar(xpdev, 0);
        return ret;
    }

    // Bus Master Enable
    ret = pci_enable_device(xpdev->pdev);
    if (ret < 0) {
        printk(KERN_CRIT"%s: getResource: Device not enabled.\n", DEVICE_NAME);
        return ret;
    }

    // Set DMA Mask
    ret = pci_set_dma_mask(xpdev->pdev, 0x7FFFFFFFFFFFFFFF);
    if (ret < 0) {
        printk(KERN_CRIT"%s: getResource: DMA not supported\n", DEVICE_NAME);
        return ret;
    }
    pci_set_consistent_dma_mask(xpdev->pdev, 0x7FFFFFFFFFFFFFFF);

    xpdev->descChain = dma_alloc_coherent(&xpdev->pdev->dev, BUF_SIZE, &xpdev->descChainHWAddr, GFP_KERNEL);
    if (NULL == xpdev->descChain) {
        printk(KERN_CRIT"%s: getResource: Unable to allocate descChain\n", DEVICE_NAME);
        return CRIT_ERR;
    }
    
    xpdev->buf = dma_alloc_coherent(&xpdev->pdev->dev, BUF_SIZE, &xpdev->bufHWAddr, GFP_KERNEL);
    if (NULL == xpdev->buf) {
        printk(KERN_CRIT"%s: getResource: Unable to allocate buf\n", DEVICE_NAME);
        return CRIT_ERR;
    }

    return SUCCESS;
}

static void XdmaFreeResource(XdmaPciDev *xpdev)
{
    UnmapSingleBar(xpdev, 0);
    UnmapSingleBar(xpdev, 1);
                
    if (NULL != xpdev->descChain) {
        dma_free_coherent(&xpdev->pdev->dev, BUF_SIZE, xpdev->descChain, xpdev->descChainHWAddr);
        xpdev->descChain = NULL;
    }
    
    if (NULL != xpdev->buf) {
        dma_free_coherent(&xpdev->pdev->dev, BUF_SIZE, xpdev->buf, xpdev->bufHWAddr);
        xpdev->buf = NULL;
    }
}

static int XdmaCreateXcdev(XdmaPciDev *xpdev, enum CDEV_TYPE type)
{
    ///NOTE::设备节点构建
    int ret = 0;
    XdmaCharDev *xcdev = &xpdev->xcdev[type];
    
    xcdev->xpdev = xpdev;
    xcdev->cdev.owner = THIS_MODULE;
    
    ret = kobject_set_name(&xcdev->cdev.kobj, g_devnodeNames[type], xpdev->boardName, xpdev->slot);
    if (ret < 0) {
        printk(KERN_WARNING"kobject set name failed\n");
        return ret;
    }
    
    switch (type) {
    case CHAR_IN :
        cdev_init(&xcdev->cdev, &g_xdmaInOutOps);
        xcdev->cdevno = MKDEV(xpdev->major, type);
        ret = cdev_add(&xcdev->cdev, xcdev->cdevno, 1);
        if (ret < 0) {
            printk(KERN_WARNING"cdev_add failed %d.\n", ret);
            return ret;
        }
        break;
        
    case CHAR_OUT :
        cdev_init(&xcdev->cdev, &g_xdmaInOutOps);
        xcdev->cdevno = MKDEV(xpdev->major, type);
        ret = cdev_add(&xcdev->cdev, xcdev->cdevno, 1);
        if (ret < 0) {
            printk(KERN_WARNING"cdev_add failed %d.\n", ret);
            return ret;
        }
        break;
    default :
        return -1;
    }
    
    printk(KERN_INFO"cdev added, major %d, minor %d\n", xpdev->major, type);
    
    xcdev->sys_device = device_create(g_xdmaClass, &xpdev->pdev->dev, xcdev->cdevno,
                                      NULL, g_devnodeNames[type], xpdev->boardName, xpdev->slot);
    if (!xcdev->sys_device) {
        printk(KERN_WARNING"device_create(%s) failed\n", g_devnodeNames[type]);
        goto del_cdev;
    }
    
    xcdev->valid = 1;    
    return 0;
    
del_cdev :
    cdev_del(&xcdev->cdev);
    return ret;
}

static void XdmaFreeXcdev(XdmaCharDev *xcdev)
{
    if (xcdev->sys_device) {
		device_destroy(g_xdmaClass, xcdev->cdevno);
        xcdev->sys_device = NULL;
    }
    
    if (xcdev->valid) {
        cdev_del(&xcdev->cdev);
        xcdev->valid = 0;
    }
}

static int XdmaProbeOne(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int ret = 0;
    dev_t dev;
    BoardName *board = NULL;
    
    XdmaPciDev *xpdev = kmalloc(sizeof(*xpdev), GFP_KERNEL);
    if (!xpdev) {
        return -ENOMEM;
    }
    
    memset(xpdev, 0, sizeof(XdmaPciDev));
    
    xpdev->pdev = pdev;
    xpdev->pci_id = *id;
    xpdev->boardName = "unknown";
    
    for (board = g_boardName; board->name != NULL; board++) {
        if (board->vendor == id->vendor && board->device == id->device) {
            xpdev->boardName = board->name;
            break;
        }
    }
    
    printk(KERN_INFO"XdmaProbeOne (%s,%X,%X)\n", xpdev->boardName, id->vendor, id->device);
    
    ret = XdmaGetResource(xpdev);
    if (0 != ret) {
        return ret;
    }
    
    xpdev->slot = XdmaReadReg(xpdev, 0);
    printk("Slot Id : %d\n", xpdev->slot);
    
    ret = alloc_chrdev_region(&dev, 0, 255, DEVICE_NAME);
    if (ret) {
        printk(KERN_WARNING"unable to allocate cdev region %d.\n", ret);
        goto free_resouce;
    }
    xpdev->major = MAJOR(dev);
    
    ret = XdmaCreateXcdev(xpdev, 0);
    if (ret) {
        XdmaFreeXcdev(&xpdev->xcdev[0]);
        goto unreg_region;
    }
    
    ret = XdmaCreateXcdev(xpdev, 1);
    if (ret) {
        XdmaFreeXcdev(&xpdev->xcdev[0]);
        goto unreg_region;
    }

    dev_set_drvdata(&pdev->dev, xpdev);
    return 0;
    
unreg_region :    
    unregister_chrdev_region(dev, 255);  

free_resouce :
    XdmaFreeResource(xpdev);
    return ret;
}

static void XdmaRemoveOne(struct pci_dev *pdev)
{
    XdmaPciDev *xpdev = dev_get_drvdata(&pdev->dev);
    struct pci_device_id *id = &xpdev->pci_id;
    
    printk(KERN_INFO"XdmaRemoveOne (%s,%X,%X)\n", xpdev->boardName, id->vendor, id->device);
    
    XdmaFreeXcdev(&xpdev->xcdev[0]);
    XdmaFreeXcdev(&xpdev->xcdev[1]);
    
    XdmaFreeResource(xpdev);
    
    kfree(xpdev);
    dev_set_drvdata(&pdev->dev, NULL);
}

static struct pci_driver pciDriver = {
	.name = DEVICE_NAME,
	.id_table = pciIds,
	.probe = XdmaProbeOne,
	.remove = XdmaRemoveOne
};

static int XdmaInit(void)
{
    g_xdmaClass = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(g_xdmaClass)) {
        printk(KERN_WARNING"Failed to create class"DEVICE_NAME);
        return -1;
    }
    
    return pci_register_driver(&pciDriver);
}

static void XdmaExit(void)
{
    pci_unregister_driver(&pciDriver);
    
    if (g_xdmaClass) {
        class_destroy(g_xdmaClass);
    }
}

module_init(XdmaInit);
module_exit(XdmaExit);
