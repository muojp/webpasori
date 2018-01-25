#pragma once

extern int webpasori_openusb(int productId, int vendorIdS310, int vendorIdS320, int vendorIdS330);
extern int webpasori_get_reader_type(void);
extern int webpasori_closeusb(void);
extern int webpasori_get_endpoints(void);
extern int webpasori_claim_interface(int num);
extern int webusb_rw_transfer_out(unsigned char *data, int size);
extern int webusb_rw_transfer_in(unsigned char *data, int size);
