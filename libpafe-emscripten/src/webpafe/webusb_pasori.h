#pragma once

extern int webpasori_openusb(int vendorId, int productIdS310, int productIdS320, int productIdS330);
extern int webpasori_get_reader_type(void);
extern int webpasori_closeusb(void);
extern int webpasori_get_endpoints(void);
extern int webpasori_select_configuration(int num);
extern int webpasori_claim_interface(int num);
extern int webusb_rw_transfer_out(unsigned char *data, int size);
extern int webusb_rw_transfer_in(unsigned char *data, int size);
extern int webusb_control_transfer_out(
    char *requestType, char *recipient, unsigned char request,
    unsigned short value, unsigned short index, unsigned char *data, int size);
