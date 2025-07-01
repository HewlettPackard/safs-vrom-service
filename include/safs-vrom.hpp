/*  safs-vrom.h
 * 
 *  Copyright 2023 Hewlett Packard Enterprise Development, LP
 *
 *  Hewlett-Packard and the Hewlett-Packard logo are trademarks of
 *  Hewlett-Packard Development Company, L.P. in the U.S. and/or other countries.
 *
 *  Confidential computer software. Valid license from Hewlett Packard Enterprise is required for
 *  possession, use or copying. Consistent with FAR 12.211 and 12.212,
 *  Commercial Computer Software, Computer Software Documentation, and Technical
 *  Data for Commercial Items are licensed to the U.S. Government under
 *  vendor's standard commercial license.
 */
#ifndef SAFS_VROM_H
#define SAFS_VROM_H

#define VROM_SYSFS_PATH                          "/sys/class/soc/kw_reg_list/vromoff"
#define FLASH_ESPIFCPRR_REGION_0_SYSFS_PATH      "/sys/class/soc/kw_reg_list/espifcprr0"
#define FLASH_ESPIFCPRR_REGION_1_SYSFS_PATH      "/sys/class/soc/kw_reg_list/espifcprr1"
#define FLASH_ESPIFCPRR_REGION_2_SYSFS_PATH      "/sys/class/soc/kw_reg_list/espifcprr2"
#define FLASH_ESPIFCPRR_REGION_3_SYSFS_PATH      "/sys/class/soc/kw_reg_list/espifcprr3"
#define FLASH_ESPIFCPRR_REGION_4_SYSFS_PATH      "/sys/class/soc/kw_reg_list/espifcprr4"
#define FLASH_ESPIFCPRR_REGION_5_SYSFS_PATH      "/sys/class/soc/kw_reg_list/espifcprr5"
#define FLASH_ESPIFCPRR_REGION_6_SYSFS_PATH      "/sys/class/soc/kw_reg_list/espifcprr6"
#define FLASH_ESPIFCPRR_REGION_7_SYSFS_PATH      "/sys/class/soc/kw_reg_list/espifcprr7"
#define FLASH_ESPIFCRAP_REG_0_SYSFS_PATH         "/sys/class/soc/kw_reg_list/espifcrap0"
#define FLASH_ESPIGCFG_SYSFS_PATH                "/sys/class/soc/kw_reg_list/espigcfg"

#define HOSTCMD_PATH                             "/sys/class/soc/kw_reg_list/hostcmd"
#define HOSTBOOT_EN_PATH                         "/sys/class/soc/fn2/fn2_host_boot_en"
#define PCI_VENDOR_ID_PATH                       "/sys/class/soc/fn2/fn2_pci_vendor"


#define MAX_DESC_REGIONS                         16
#define MAX_PROTECTED_RANGE_REGS                 8
#define FCCFGVALID                               16
#define BUF_SIZE_512B                            512

// structure describing Flash descriptor content
#pragma pack(1)
typedef union {
    struct {
        uint32_t region_base : 15;
        uint32_t reserved1 : 1;
        uint32_t region_limit : 15;
        uint32_t reserved2 : 1;
    } bits;
    uint32_t u32;
} FLASH_REGION_REG;

typedef struct {
    uint8_t                    reserved1[16];
    uint32_t                   signature;
    uint32_t		       space1[3];
    uint8_t                    reserved2[16];
    uint32_t		       space2;
    uint8_t                    reserved3[12];
    FLASH_REGION_REG           flash_region_reg_arr[MAX_DESC_REGIONS];
} flash_descriptor;
#pragma pack()

#endif
