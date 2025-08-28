/**********************************************************************************************************
*
*  safs-vrom.cpp
*
*  Copyright 2023 Hewlett Packard Enterprise Development, LP
*
*  Hewlett-Packard and the Hewlett-Packard logo are trademarks of Hewlett-Packard Development Company, L.P.
*  in the U.S. and/or other countries.
*
*  Confidential computer software. Valid license from Hewlett Packard Enterprise is required for possession, use or
*  copying. Consistent with FAR 12.211 and 12.212, Commercial Computer Software, Computer Software
*  Documentation, and Technical Data for Commercial Items are licensed to the U.S. Government under
*  vendor's standard commercial license.
*
**********************************************************************************************************/
#include <iostream>
#include <fstream>
#include <cmath>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>
#include "safs-vrom.hpp"
#include <phosphor-logging/lg2.hpp>
#include <cstring>

using namespace std;


int update_sysfs_reg32(const char *path, uint32_t val, bool overwrite)
{
    FILE *fp = NULL;
    char rwstr[16];
    int rv = 1;
    uint32_t prev_val = 0;

    fp = fopen(path,"r+");
    if (fp == NULL) {
        lg2::emergency("unable to open SYSFS path {PATH}", "PATH" , std::string(path));
        goto end;
    }
    if (!overwrite) {
        if (fgets(rwstr, sizeof(rwstr), fp) == NULL) {
            lg2::emergency("unable to read from SYSFS path {PATH}", "PATH" , std::string(path));
            goto end;
        }
        sscanf(rwstr+2,"%x",&prev_val);
    }
    sprintf(rwstr,"0x%x", (prev_val | val));
    lg2::info("Updating {PATH} from {PREV} to {NEW}",
              "PATH" , std::string(path), "PREV" , lg2::hex, prev_val, "NEW", std::string(rwstr));
    fputs(rwstr,fp);
    fflush(fp);
    rv = 0;

end:
    if (fp)
        (void) fclose(fp);
    return rv;
}


bool enable_and_copy_vrom_content()
{
    const char *host_mtd_path = MTD_BY_NAME_HOST_PRIME_PATH;
    const char *vrom_mtd_path = MTD_BY_NAME_VROM_PRIME_PATH;

    char buf[BUF_SIZE_512B];
    char str[20];
    char *tmp;
    uint32_t vrom_offset_hex;
    int ioctl_error, cur;
    uint64_t index = 0;
    mtd_info_t mtd_info_host, mtd_info_vrom;
    size_t rd_count = 0, wr_count = 0, total_rd_cnt = 0;
    FILE *fp;

    FILE *fd_host = fopen(host_mtd_path, "rb");
    FILE *fd_vrom = fopen(vrom_mtd_path, "wb");
    if (fd_host == NULL || fd_vrom == NULL)
    {
        lg2::emergency("unable to open MTD device path");
        return false;
    }
    ioctl_error = ioctl(fd_host->_fileno, MEMGETINFO, &mtd_info_host);
    if (ioctl_error == -1)
    {
        lg2::emergency("error during ioctl for MEMGETINFO command for mtd path : {HOST_MTD_PATH} and errno : {ERRNO}" , "HOST_MTD_PATH" , std::string(host_mtd_path) ,"ERRNO" , errno);
        fclose(fd_host);
        fclose(fd_vrom);
        return false;
    }
    ioctl_error = ioctl(fd_vrom->_fileno, MEMGETINFO, &mtd_info_vrom);
    if (ioctl_error == -1)
    {
        lg2::emergency("error during ioctl for MEMGETINFO command for mtd path : {VROM_MTD_PATH} and errno : {ERRNO}" , "VROM_MTD_PATH" , std::string(vrom_mtd_path) ,"ERRNO" , errno);
        fclose(fd_host);
        fclose(fd_vrom);
        return false;
    }
    if (mtd_info_host.size == mtd_info_vrom.size)
    {
        cur = fseek(fd_host, 0, SEEK_SET);
        if (cur == -1)
        {
            lg2::emergency("error during lseek operation from MTD partition : {HOST_MTD_PATH} and errno : {ERRNO}" , "HOST_MTD_PATH" , std::string(host_mtd_path) ,"ERRNO" , errno);
            fclose(fd_host);
            fclose(fd_vrom);
            return false;
        }
        cur = fseek(fd_vrom, 0, SEEK_SET);
        if (cur == -1)
        {
            lg2::emergency("error during lseek operation from MTD partition : {VROM_MTD_PATH} and errno : {ERRNO}" , "VROM_MTD_PATH" , std::string(vrom_mtd_path) ,"ERRNO" , errno);
            fclose(fd_host);
            fclose(fd_vrom);
            return false;
        }
        // copy BIOS Content from SPI to VROM
        for (index = 0;index < mtd_info_host.size; index += total_rd_cnt)
        {
            rd_count = fread(buf, 1, sizeof(buf), fd_host);
            total_rd_cnt = rd_count;
            if(rd_count <= 0)
            {
                lg2::emergency("error during reading BIOS content from MTD partition : {HOST_MTD_PATH} and errno : {ERRNO}" , "HOST_MTD_PATH" , std::string(host_mtd_path) ,"ERRNO" , errno);
                fclose(fd_host);
                fclose(fd_vrom);
                return false;
            }
            wr_count = 0;
            do
            {
                wr_count = fwrite(buf + wr_count, 1, rd_count, fd_vrom);
                if (wr_count <= 0)
                {
                    lg2::emergency("error during writing BIOS content to MTD partition : {VROM_MTD_PATH} and errno : {ERRNO}" , "VROM_MTD_PATH" , std::string(vrom_mtd_path) ,"ERRNO" , errno);
                    fclose(fd_host);
                    fclose(fd_vrom);
                    return false;
                }
                rd_count = rd_count - wr_count;
            } while(rd_count > 0);
        }
    }
    else
    {
        lg2::emergency("HOST and VROM MTD partition size mismatch, Host MTD size : {HOST_MTD_SIZE} and VROM MTD Size : {VROM_MTD_SIZE}" , "HOST_MTD_SIZE" , mtd_info_host.size ,"VROM_MTD_SIZE" , mtd_info_vrom.size);
        fclose(fd_host);
        fclose(fd_vrom);
        return false;
    }
    fclose(fd_host);
    fclose(fd_vrom);

    // enable VROM Support
    fp = fopen(VROM_SYSFS_PATH,"r+");
    if (fp == NULL){
        lg2::emergency("unable to open VROM SYSFS path  {VROM_PATH} , VROM will be disabled" , "VROM_PATH" , std::string(VROM_SYSFS_PATH));
        return false;
    }
    tmp = fgets(str,sizeof(str),fp);
    if(tmp == NULL)
    {
        fclose(fp);
        return false;
    }
    sscanf(str+2,"%x",&vrom_offset_hex);
    vrom_offset_hex = vrom_offset_hex | (1 << 3);
    sprintf(str,"0x%x",vrom_offset_hex);
    fputs(str,fp);
    fclose(fp);
    return true;
}

int main()
{
    bool rc;
    const char *mtd_path = MTD_BY_NAME_HOST_PRIME_PATH;
    char flash_descriptor_data[sizeof(flash_descriptor)];
    flash_descriptor *desc_data = NULL;
    uint32_t reg_base = 0;
    uint32_t reg_limit = 0;
    uint32_t prr_data = 0, rap_data = 0;
    uint8_t  used_reg_idx = 0;
    uint8_t  reg_idx = 0;
    FILE     *fd_host, *fp;
    char     str[20];
    int cur;
    size_t rd_count;
    const char *prr_sysfs_address[MAX_PROTECTED_RANGE_REGS] = {FLASH_ESPIFCPRR_REGION_0_SYSFS_PATH,
                                                               FLASH_ESPIFCPRR_REGION_1_SYSFS_PATH,
                                                               FLASH_ESPIFCPRR_REGION_2_SYSFS_PATH,
                                                               FLASH_ESPIFCPRR_REGION_3_SYSFS_PATH,
                                                               FLASH_ESPIFCPRR_REGION_4_SYSFS_PATH,
                                                               FLASH_ESPIFCPRR_REGION_5_SYSFS_PATH,
                                                               FLASH_ESPIFCPRR_REGION_6_SYSFS_PATH,
                                                               FLASH_ESPIFCPRR_REGION_7_SYSFS_PATH};

    // find MTD device for HOST and VROM and copy contents from HOST to VROM and enable VROM
    rc = enable_and_copy_vrom_content();
    if (rc == false)
    {
        lg2::emergency("Error while enabling and copying VROM Contents and rc: {ERROR}", "ERROR", rc);
        exit(EXIT_FAILURE);
    }
    lg2::info("VROM is enabled");

    // Parse combo flash to find the base address and limit of different regions and program flash channel region protection registers
    // eSPI flash channel Protected Range Register defination
    // [15:0] - Regions Base
    // [31:16] - Region Limit
    fd_host = fopen(mtd_path, "rb");
    if (fd_host == NULL)
    {
        lg2::emergency("unable to open HOST MTD device path, errno: {ERROR}", "ERROR", errno);
        exit(EXIT_FAILURE);
    }
    memset(flash_descriptor_data,0x00,sizeof(flash_descriptor_data));
    do
    {
        cur = fseek(fd_host, 0, SEEK_SET);
        if (cur == -1)
        {
            lg2::emergency("error during lseek operation from MTD partition : {MTD_PATH} and errno : {ERRNO}" , "MTD_PATH" , std::string(mtd_path) ,"ERRNO" , errno);
            fclose(fd_host);
            exit(EXIT_FAILURE);
        }
        rd_count = fread(flash_descriptor_data, 1, sizeof(flash_descriptor_data), fd_host);
        if(rd_count <= 0)
        {
            lg2::emergency("error during reading flash decriptor Data from MTD partition : {MTD_PATH} and errno : {ERRNO}" , "MTD_PATH" , std::string(mtd_path) ,"ERRNO" , errno);
            fclose(fd_host);
            exit(EXIT_FAILURE);
        }
    } while(rd_count != sizeof(flash_descriptor_data));
    fclose(fd_host);

    // check for valid flash signature and if signature value is 0FF0_A55A, then flash descriptor is valid. if it's not valid then exit
    desc_data = (flash_descriptor *)flash_descriptor_data;
    if(desc_data->signature == 0x0FF0A55A)
    {
        lg2::info("Found Valid Flash Signature and flash descriptor is valid");
    }
    else
    {
        lg2::emergency("Invalid Flash Signature and flash descriptor is not valid, signature : {SIGNATURE}", "SIGNATURE", desc_data->signature);
        exit(EXIT_FAILURE);
    }
    // if region base is 0x7FFF and region limit is 0x0000, then the region is not used, otherwise region is used.
    for(reg_idx = 0; reg_idx < MAX_DESC_REGIONS; reg_idx++)
    {
        reg_base = desc_data->flash_region_reg_arr[reg_idx].bits.region_base;
        reg_limit = desc_data->flash_region_reg_arr[reg_idx].bits.region_limit;
        if((reg_base == 0x7FFF) && (reg_limit == 0x0000))
        {
            lg2::info("Flash descriptor Region {REGID} is not used", "REGID", reg_idx);
            continue;
        }
        else
        {
            lg2::info("Flash descriptor Region {REGID} is used,region base: {REGBASE} and region limit :{REGLIMIT}" , "REGID" , reg_idx ,"REGBASE" , reg_base, "REGLIMIT", reg_limit);
            prr_data = (reg_base & 0xFFFF) | (((reg_limit & 0xFFFF) + 1) << 16);
            fp = fopen(prr_sysfs_address[used_reg_idx],"w");
            if (fp == NULL){
                lg2::emergency("unable to open ESPIFCPRR SYSFS path {SYSFS_PATH}", "SYSFS_PATH", std::string(prr_sysfs_address[used_reg_idx]));
                exit(EXIT_FAILURE);
            }
            sprintf(str,"0x%08x",prr_data);
            fputs(str,fp);
            fclose(fp);
            used_reg_idx++;
            if (used_reg_idx >= MAX_PROTECTED_RANGE_REGS)
            {
                lg2::emergency("All protected Range registers are already being used and used reg index : {REGID}", "REGID", used_reg_idx);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (used_reg_idx == 0x00)
    {
        lg2::emergency("none of the regions are used");
        exit(EXIT_FAILURE);
    }

    // program e-SPI Flash channel range access protection registers
    fp = fopen(FLASH_ESPIFCRAP_REG_0_SYSFS_PATH,"w");
    if (fp == NULL){
        lg2::emergency("unable to open ESPIFCRAP reg 0 SYSFS path {SYSFS_PATH}" , "SYSFS_PATH" , std::string(FLASH_ESPIFCRAP_REG_0_SYSFS_PATH));
        exit(EXIT_FAILURE);
    }
    rap_data = (((int)(pow(2,used_reg_idx) - 1) << 16) | 0x000f);
    sprintf(str,"0x%08x",rap_data);
    fputs(str,fp);
    fclose(fp);

    //enable flash channel access
    update_sysfs_reg32(FLASH_ESPIGCFG_SYSFS_PATH, (1 << FCCFGVALID), false);

    // enable host boot
    update_sysfs_reg32(HOSTBOOT_EN_PATH, 1, true);

    // set subsystem and vendor id
    update_sysfs_reg32(PCI_VENDOR_ID_PATH, 0x03d91590, true);

    return 0;
}
