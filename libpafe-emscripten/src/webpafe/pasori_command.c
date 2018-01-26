/* $Id: pasori_command.c,v 1.11 2009-10-09 07:43:13 hito Exp $ */
/* pasori commands */
#ifdef HAVE_CONFIG_H 
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libpafe.h"

#ifdef HAVE_LIBUSB_1
#include <libusb.h>
#define INTERFACE_NUMBER 0
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
#include <emscripten.h>
#include "webusb_pasori.h"
#define INTERFACE_NUMBER 0
#else  /* HAVE_LIBUSB_1 */
#include <usb.h>
#endif	/* HAVE_LIBUSB_1 */

struct tag_pasori
{
#ifdef HAVE_LIBUSB_1
  libusb_device **devs;
  libusb_context *ctx;
  libusb_device_handle *dh;
  struct libusb_device_descriptor desc;
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
#else  /* HAVE_LIBUSB_1 */
  struct usb_device *dev;
  usb_dev_handle *dh;
#endif	/* HAVE_LIBUSB_1 */
  int b_ep_in, b_ep_out, i_ep_in;
  int timeout;
  enum PASORI_TYPE type;
};

#define PASORIUSB_VENDOR 0x054c
#define PASORIUSB_PRODUCT_S310 0x006c
#define PASORIUSB_PRODUCT_S320 0x01bb
#define PASORIUSB_PRODUCT_S330 0x02e1

#define TIMEOUT 1000

/* FIXME: UNKNOWN CONSTANTS */
static const uint8 S320_INIT0[] = { 0x62, 0x01, 0x82 };
/* RET0                      {0x63,0x00,0x88}; */

static const uint8 S320_INIT1[] = { 0x62, 0x02, 0x80, 0x81 };	/* INIT3 */
/* RET1                      {0x63,0x00,0xcc,0x88}; */

static const uint8 S320_INIT2[] = { 0x62, 0x22, 0x80, 0xcc, 0x81, 0x88 };
/* RET2                      {0x63,0x00}; */

static const uint8 S320_INIT3[] = { 0x62, 0x02, 0x80, 0x81 };	/* INIT1 */
/* RET3                      {0x63,0x00,0xcc,0x88}; */

static const uint8 S320_INIT4[] = { 0x62, 0x02, 0x82, 0x87 };
/* RET4                      {0x63,0x00,0x88,0x01}; */

static const uint8 S320_INIT5[] = { 0x62, 0x21, 0x25, 0x58 };
/* RET5                      {0x63,0x00} */

static const uint8 S320_READ0[] = { 0x58 };
/*RRET0                      {0x59,0x28,0x01} */

static const uint8 S320_READ1[] = { 0x54 };
static const uint8 S320_READ2[] = { 0x5a, 0x80 };

static const uint8 S310_INIT[] = { 0x54 };

static const uint8 S330_RF_ANTENNA_ON[] = { 0xD4, 0x32, 0x01, 0x01 };
static const uint8 S330_RF_ANTENNA_OFF[] = { 0xD4, 0x32, 0x01, 0x00 };
static const uint8 S330_GET_VERSION[] = { 0xD4, 0x02 };
static const uint8 S330_DESELECT[] = { 0xD4, 0x44, 0x01 };


#define DATASIZE 255

/* internal */

static int
checksum(uint8 *data, int size)
{
  int i, sum = 0;

  if (data == NULL)
    return 0;

  for (i = 0; i != size; i++) {
    sum += data[i];
  }
  sum &= 0xff;
  sum = 0x100 - sum;

  return sum & 0xff;
}

static int
pasori_init_test(pasori *p, const uint8 *testptrn, int size)
{
  uint8 recv[DATASIZE + 1];
  int n, r;

  if (p == NULL || testptrn == NULL || size < 1)
    return PASORI_ERR_PARM;

  n = size;
  r = pasori_packet_write(p, (uint8 *) testptrn, &n);
  if (r)
    return r;

  n = DATASIZE;
  r = pasori_recv(p, recv, &n);

  return r;
}

/* exports */

int 
pasori_test(pasori *p, int code, uint8 *data, int *size, uint8 *rdata, int *rsize)
{
  uint8 recv[DATASIZE + 1];
  int n, r;


  if (p == NULL || size == NULL)
    return PASORI_ERR_PARM;

  if (code == 0x00 && (size ==NULL ||rdata == NULL || rsize == NULL))
    return PASORI_ERR_PARM;

  switch (p->type) {
  case PASORI_TYPE_S310:
  case PASORI_TYPE_S320:
    break;
  default:
    return PASORI_ERR_TYPE;
  }

  n = *size;

  if (n > DATASIZE - 3)
    return PASORI_ERR_PARM;

  recv[0] = 0x52;
  recv[1] = code;
  recv[2] = *size;
  if (n > 0) {
    memcpy(recv + 3, data, n);
  }
  n += 3;

  r = pasori_packet_write(p, recv, &n);
  if (r)
    return r;

  n = DATASIZE;
  r = pasori_packet_read(p, recv, &n);
  if (r)
    return r;

  if (recv[0] != 0x53)
    return PASORI_ERR_FORMAT;

  n = recv[1];
  if (code != 0x00 && n != 1) {
    return n;
  }

  if (code != 0x00)
    return 0;

  if (n > *rsize)
    n = *rsize;

  recv[2 + n] = '\0';
  memcpy(rdata, &recv[2], n);
  *rsize = n;
  return 0;
}

int 
pasori_test_echo(pasori *p, uint8 *data, int *size)
{
  int n = *size, l = DATASIZE, r;
  uint8 rdata[DATASIZE + 1];

  r = pasori_test(p, 0x00, data, &n, rdata, &l);
  if (r)
    return r;

  if (n != l)
    return PASORI_ERR_DATA;

  if (memcmp(data, rdata, n))
    return PASORI_ERR_DATA;

  return 0;
}

int 
pasori_test_eprom(pasori *p)
{
  uint8 recv[DATASIZE + 1];
  int n = 0, rn = DATASIZE;
  
  return pasori_test(p, 0x01, NULL, &n, recv, &rn);
}

int 
pasori_test_ram(pasori *p)
{
  int n = 0;
  
  return pasori_test(p, 0x02, NULL, &n, NULL, NULL);
}

int 
pasori_test_cpu(pasori *p)
{
  int n = 0;
  
  return pasori_test(p, 0x03, NULL, &n, NULL, NULL);
}

int 
pasori_test_polling(pasori *p)
{
  int n = 0;
  
  return pasori_test(p, 0x10, NULL, &n, NULL, NULL);
}



void 
pasori_set_timeout(pasori *p, int timeout)
{
  if (p == NULL || timeout < 0)
    return;

  p->timeout = timeout;
}

int
pasori_packet_write(pasori *p, uint8 *data, int *size)
{				/* RAW Packet SEND */
  uint8 cmd[DATASIZE + 1];
  uint8 sum;
  int i, n;

  if (p == NULL || data == NULL || size == NULL)
    return PASORI_ERR_PARM;

  n = *size;

  if (n < 1) {
    *size = 0;
    return 0;
  }

  if (n > DATASIZE - 7)
    n = DATASIZE - 7;

  sum = checksum(data, n);

  cmd[0] = 0;
  cmd[1] = 0;;
  cmd[2] = 0xff;
  cmd[3] = n;
  cmd[4] = 0x100 - n;
  memcpy(cmd + 5, data, n);
  cmd[5 + n] = sum;
  cmd[6 + n] = 0;
  n += 7;

  i = pasori_send(p, cmd, &n);

  *size = n - 7;
  return i;
  /* FIXME:handle error */
}

int
pasori_packet_read(pasori * p, uint8 * data, int *size)
{
  uint8 recv[DATASIZE + 1];
  unsigned int s;
  int i, n, sum;

  if (p == NULL || data == NULL || size == NULL)
    return PASORI_ERR_PARM;

  if (*size < 1) {
    *size = 0;
    return 0;
  }

  n = DATASIZE;
  i = pasori_recv(p, recv, &n);

  if (i)
    return i;			/* FIXME: handle timeout */

  if (recv[0] != 0 || recv[1] != 0 || recv[2] != 0xff)
    return PASORI_ERR_COM;

  if (recv[5] == 0x7f)
    return PASORI_ERR_FORMAT;

  s = recv[3];
  if (recv[4] != 0x100 - s)
    return PASORI_ERR_CHKSUM;

  sum = checksum(recv + 5, s);
  if (recv[s + 5] != sum)
    return PASORI_ERR_CHKSUM;

  if (recv[s + 6] != 0)
    return PASORI_ERR_COM;

  if (s > n)
    s = n;

  memcpy(data, &recv[5], s);
  *size = s;

  return 0;
}

/*
 * 3th byte is the max number of target to sence.  
 * It seems valid values are 0x01 or 0x02 for devices this driver suport, and
 * it can sences two targets at Mifare cards, but not at Felica cards.  
 * 
 * 4th byte may mean protocol(tag types).
 *   0x00 = mifare,iso14443A(106kbps*)
 *   0x01 = Felica(212kbps)
 *   0x02 = Felica(424kbps)
 *   0x03 = iso14443B(106kbps*)
 *   0x04 = Nfc Forum Type1(106kbps*)
 *   *=default bit rate(To change this, use pn53x 'D44E' command.)
 * 
 * At least on default setting, it seems my pn531 dose not accept '0x03''0x04'.
 * RC-S330 accepts '0x03''0x04', but in the '0x03' case payload length is up to 
 * 2 byte, in the '0x04' case payload length is zero , and in both case
 * 3th byte must be '0x01'.
 * 
 * */
int 
pasori_list_passive_target(pasori *pp, unsigned char *payload, int *size)
{
  int r, n;
  unsigned char cmd[DATASIZE + 1];

  if(pp == NULL || payload == NULL || size == NULL || *size < 0) 
    return PASORI_ERR_FORMAT;

  if (pp->type != PASORI_TYPE_S330)
    return PASORI_ERR_TYPE;

  cmd[0] = 0xd4;
  cmd[1] = 0x4a;
  cmd[2] = 1;
  cmd[3] = 0x01;

  n = *size;
  memcpy(cmd + 4, payload, n);
  n += 4;
#if defined(HAVE_WEBUSB)
  // FIXME: inspect why removing this printf make polling functionality broken
  printf("");
#endif
  r = pasori_packet_write(pp, cmd, &n);
  *size = n - 4;

  return r;			/* FIXME:handle error */
}

int
pasori_write(pasori *p, uint8 *data, int *size)
{
  uint8 cmd[DATASIZE];
  int r, n, head_len;

  n = *size;

  if (n > DATASIZE - 2)
    return PASORI_ERR_PARM;

  switch (p->type) {
  case PASORI_TYPE_S310:
  case PASORI_TYPE_S320:
    cmd[0] = 0x5c;
    cmd[1] = *size + 1;
    head_len = 2;
    break;
  case PASORI_TYPE_S330:
    cmd[0] = 0xd4;
    cmd[1] = 0x42;
    cmd[2] = *size + 1;
    head_len = 3;
    break;
  default:
    return PASORI_ERR_TYPE;
  }

  memcpy(cmd + head_len, data, n);
  n += head_len;
  r = pasori_packet_write(p, cmd, &n);
  *size = n - head_len;

  return r;			/* FIXME:handle error */
}

int 
pasori_read(pasori *p, uint8 *data, int *size)
{
  uint8 recv[DATASIZE + 1];
  int s;
  int n, r;

  if (p == NULL || data == NULL || size == NULL)
    return PASORI_ERR_PARM;

  if (*size < 1) {
    *size = 0;
    return 0;
  }

  n = DATASIZE;
  r = pasori_packet_read(p, recv, &n);
  if (r)
    return r;

  switch (p->type) {
  case PASORI_TYPE_S310:
  case PASORI_TYPE_S320:
    if (recv[0] != 0x5d)
      return PASORI_ERR_FORMAT;
    s = recv[1];
    break;
  case PASORI_TYPE_S330:
    if (recv[0] != 0xd5)
      return PASORI_ERR_FORMAT;
    s = n;
    break;
  default:
    return PASORI_ERR_TYPE;
  }

  if (s > *size)
    s = *size;

  memcpy(data, recv + 2, s);
  *size = s;

  return 0;
}

int
pasori_init(pasori * p)
{
  if (p == NULL)
    return PASORI_ERR_PARM;

  switch (p->type) {
  case PASORI_TYPE_S310:
    pasori_init_test(p, S310_INIT, sizeof(S310_INIT));
    break;
  case PASORI_TYPE_S320:
    pasori_init_test(p, S320_INIT0, sizeof(S320_INIT0));
    pasori_init_test(p, S320_INIT1, sizeof(S320_INIT1));
    pasori_init_test(p, S320_INIT2, sizeof(S320_INIT2));
    pasori_init_test(p, S320_INIT3, sizeof(S320_INIT3));
    pasori_init_test(p, S320_INIT4, sizeof(S320_INIT4));
    pasori_init_test(p, S320_INIT5, sizeof(S320_INIT5));

    pasori_init_test(p, S320_READ2, sizeof(S320_READ2));
    break;
  case PASORI_TYPE_S330:
    pasori_init_test(p, S330_RF_ANTENNA_ON, sizeof(S330_RF_ANTENNA_ON));
    break;
  default:
    return PASORI_ERR_TYPE;
  }
  return 0;
}

int
pasori_reset(pasori * p)
{
  if (p == NULL)
    return PASORI_ERR_PARM;

  switch (p->type) {
  case PASORI_TYPE_S310:
    pasori_init_test(p, S310_INIT, sizeof(S310_INIT));
    break;
  case PASORI_TYPE_S320:
    pasori_init_test(p, S320_READ1, sizeof(S320_READ1));
    break;
  case PASORI_TYPE_S330:
    pasori_init_test(p, S330_DESELECT, sizeof(S330_DESELECT));
    pasori_init_test(p, S330_RF_ANTENNA_OFF, sizeof(S330_RF_ANTENNA_ON));
    break;
  default:
    return PASORI_ERR_TYPE;
  }

  return 0;
}

static int
bcd2int(uint8 bcd)
{
  return ((bcd >> 4) & 0x0f) * 10 + (bcd & 0x0f);
}

int
pasori_version(pasori *p, int *v1, int *v2)
{
  uint8 recv[DATASIZE + 1];
  int n, r;

  if (p == NULL || v1 == NULL || v2 == NULL)
    return PASORI_ERR_PARM;

  switch (p->type) {
  case PASORI_TYPE_S310:
  case PASORI_TYPE_S320:
    recv[0] = 0x58;
    n = 1;
    break;
  case PASORI_TYPE_S330:
    recv[0] = 0xd4;
    recv[1] = 0x02;
    n = 2;
    break;
  default:
    return PASORI_ERR_TYPE;
  }

  r = pasori_packet_write(p, recv, &n);
  if (r)
    return r;

  n = DATASIZE;
  r = pasori_packet_read(p, recv, &n);
  if (r)
    return r;

  switch (p->type) {
  case PASORI_TYPE_S310:
  case PASORI_TYPE_S320:
    if (recv[0] != 0x59)
      return PASORI_ERR_FORMAT;
    *v1 = recv[2];
    *v2 = recv[1];
    break;
  case PASORI_TYPE_S330:
    *v1 = bcd2int(recv[3]);
    *v2 = bcd2int(recv[4]);
    break;
  default:
    return PASORI_ERR_TYPE;
  }

  return 0;
}

int 
pasori_type(pasori *p)
{
  if (p == NULL) {
    return -1;
  }

  return p->type;
}

static void
dbg_dump(unsigned char *b, uint8 size)
{
  int i;

  for (i = 0; i != size; i++) {
    Log("%02X ", b[i]);
  }
  Log("\n");
}

void
pasori_close(pasori * pp)
{
  if (!pp)
    return;

#ifdef HAVE_LIBUSB_1
  if (pp->dh) {
    pasori_reset(pp);
    libusb_release_interface(pp->dh, INTERFACE_NUMBER);
    libusb_close(pp->dh);
  }

  if (pp->devs) {
    libusb_free_device_list(pp->devs, 1);
  }

  if (pp->ctx) {
    libusb_exit(pp->ctx);
  }
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
  webpasori_closeusb();
#else  /* HAVE_LIBUSB_1 */
  if (pp->dh) {
    pasori_reset(pp);
    usb_release_interface(pp->dh,
			  pp->dev->config->interface->altsetting->
			  bInterfaceNumber);
    usb_close(pp->dh);
  }
#endif

  free(pp);
}

static void 
get_end_points(pasori *pas)
{
#ifdef HAVE_LIBUSB_1
  struct libusb_config_descriptor *config;
  const struct libusb_interface_descriptor *interdesc;
  const struct libusb_endpoint_descriptor *epdesc;
  const struct libusb_interface *inter;
  libusb_device *dev;
  int i, j, k;

  dev = libusb_get_device(pas->dh);
  libusb_get_config_descriptor(dev, 0, &config);

  for(i = 0; i < (int) config->bNumInterfaces; i++) {
    inter = &config->interface[i];
    for(j = 0; j < inter->num_altsetting; j++) {
      interdesc = &inter->altsetting[j];
      for(k = 0; k < (int) interdesc->bNumEndpoints; k++) {
	epdesc = &interdesc->endpoint[k];
#ifdef DEBUG
	printf("Endpoint      : 0x%02X\n"
	       "Transfer Type : %x\n",
	       epdesc->bEndpointAddress,
	       epdesc->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK);
#endif
	switch (epdesc->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) {
	case LIBUSB_TRANSFER_TYPE_BULK:
	  if ((epdesc->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
#ifdef DEBUG
	    printf("Bulk endpoint in  : 0x%02X\n", epdesc->bEndpointAddress);
#endif
	    pas->b_ep_in = epdesc->bEndpointAddress;
	  }
	  if ((epdesc->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT) {
#ifdef DEBUG
	    printf("Bulk endpoint out  : 0x%02X\n", epdesc->bEndpointAddress);
#endif
	    pas->b_ep_out = epdesc->bEndpointAddress;
	  }
	  break;
	case LIBUSB_TRANSFER_TYPE_INTERRUPT:
#ifdef DEBUG
	  if ((epdesc->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
	    printf("Interrupt endpoint  : 0x%02X\n", epdesc->bEndpointAddress);
	  }
#endif
	  pas->i_ep_in = epdesc->bEndpointAddress;
	}
      }
    }
  }
  libusb_free_config_descriptor(config);
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
  webpasori_get_endpoints();
#else  /* HAVE_LIBUSB_1 */
  int uiIndex;
  int uiEndPoint;
  struct usb_interface_descriptor* puid = pas->dev->config->interface->altsetting;

  // 3 Endpoints maximum: Interrupt In, Bulk In, Bulk Out
  for(uiIndex = 0; uiIndex < puid->bNumEndpoints; uiIndex++) {
    // Copy the endpoint to a local var, makes it more readable code
    uiEndPoint = puid->endpoint[uiIndex].bEndpointAddress;

    switch (puid->endpoint[uiIndex].bmAttributes) {
    case USB_ENDPOINT_TYPE_BULK:

      // Test if we dealing with a bulk IN endpoint
      if((uiEndPoint & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_IN) {
#ifdef DEBUG
	printf("Bulk endpoint in  : 0x%02X\n", uiEndPoint);
#endif
	pas->b_ep_in = uiEndPoint;
      }

      // Test if we dealing with a bulk OUT endpoint
      if((uiEndPoint & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_OUT) {
#ifdef DEBUG
	printf("Bulk endpoint out  : 0x%02X\n", uiEndPoint);
#endif
	pas->b_ep_out = uiEndPoint;
      }
      break;
    case USB_ENDPOINT_TYPE_INTERRUPT:
      if((uiEndPoint & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_IN) {
#ifdef DEBUG
	printf("Interrupt endpoint  : 0x%02X\n", epdesc->bEndpointAddress);
#endif
	pas->i_ep_in = uiEndPoint;
      }
      break;
    }
  }
#endif	/* HAVE_LIBUSB_1 */
}

static int
open_usb(pasori *pp)
{
#ifdef HAVE_LIBUSB_1
  int i, r, cnt;
  struct libusb_device_descriptor desc;
  libusb_device *dev;

  pp->ctx = NULL;
  pp->devs = NULL;
  r = libusb_init(&pp->ctx);
  if (r < 0) {
    return PASORI_ERR_COM;
  }
#ifdef DEBUG_USB
  libusb_set_debug(pp->ctx, 3);
#endif
  cnt = libusb_get_device_list(pp->ctx, &pp->devs); //get the list of devices
  if(cnt < 0) {
    return PASORI_ERR_COM;
  }

  for(i = 0; i < cnt; i++) {
    r = libusb_get_device_descriptor(pp->devs[i], &desc);
    if (r < 0) {
      continue;
    }

#ifdef DEBUG_USB
    Log("Check for %04x:%04x\n", desc.idVendor, desc.idProduct);	/* debug */
#endif
    if (desc.idVendor == PASORIUSB_VENDOR && 
	(desc.idProduct == PASORIUSB_PRODUCT_S310 ||
	 desc.idProduct == PASORIUSB_PRODUCT_S320 ||
	 desc.idProduct == PASORIUSB_PRODUCT_S330)) {
#ifdef DEBUG_USB
      Log("Device is found %04x:%04x\n", desc.idVendor, desc.idProduct);	/* debug */
#endif
      dev = pp->devs[i];
      goto finish;
    }
  }
  Log("pasori not found in USB BUS");
  return PASORI_ERR_COM;

 finish:

  switch (desc.idProduct) {
  case PASORIUSB_PRODUCT_S310:
    pp->type = PASORI_TYPE_S310;
    break;
  case PASORIUSB_PRODUCT_S320:
    pp->type = PASORI_TYPE_S320;
    break;
  case PASORIUSB_PRODUCT_S330:
    pp->type = PASORI_TYPE_S330;
    break;
  default:
    return PASORI_ERR_TYPE;
  }

  r = libusb_open(dev, &pp->dh);
  if(r) {
    return PASORI_ERR_COM;
  }

  if(libusb_kernel_driver_active(pp->dh, 0) == 1) {
    r = libusb_detach_kernel_driver(pp->dh, 0);
    if (r) {
      return PASORI_ERR_COM;
    }
  }

  pp->timeout = TIMEOUT;
  get_end_points(pp);

  if(libusb_claim_interface(pp->dh, INTERFACE_NUMBER) < 0) {
    return PASORI_ERR_COM;
  }

  return 0;
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
  int ret = webpasori_openusb(PASORIUSB_VENDOR, PASORIUSB_PRODUCT_S310, PASORIUSB_PRODUCT_S320, PASORIUSB_PRODUCT_S330);
  if (ret != 0) {
    return PASORI_ERR_COM;
  }
  switch (webpasori_get_reader_type()) {
  case PASORIUSB_PRODUCT_S310:
    pp->type = PASORI_TYPE_S310;
    break;
  case PASORIUSB_PRODUCT_S320:
    pp->type = PASORI_TYPE_S320;
    break;
  case PASORIUSB_PRODUCT_S330:
    pp->type = PASORI_TYPE_S330;
    break;
  default:
    return PASORI_ERR_TYPE;
  }
  get_end_points(pp);
  ret = webpasori_claim_interface(INTERFACE_NUMBER);
  return 0;
#else  /* HAVE_LIBUSB_1 */
  struct usb_bus *bus;
  struct usb_device *dev;

  usb_init();
#ifdef DEBUG_USB
  usb_set_debug(255);
#else
  usb_set_debug(0);
#endif
  usb_find_busses();
  usb_find_devices();

  for (bus = usb_get_busses(); bus; bus = bus->next) {
    for (dev = bus->devices; dev; dev = dev->next) {
#ifdef DEBUG_USB
      Log("check for %04x:%04x\n", dev->descriptor.idVendor, dev->descriptor.idProduct);	/* debug */
#endif
      if (dev->descriptor.idVendor == PASORIUSB_VENDOR &&
	  (dev->descriptor.idProduct == PASORIUSB_PRODUCT_S310 ||
	   dev->descriptor.idProduct == PASORIUSB_PRODUCT_S320 ||
	   dev->descriptor.idProduct == PASORIUSB_PRODUCT_S330)) {
#ifdef DEBUG_USB
	Log("Device is found %04x:%04x\n", dev->descriptor.idVendor, dev->descriptor.idProduct);	/* debug */
#endif
	goto finish;
      }
    }
  }
  Log("pasori not found in USB BUS");
  return PASORI_ERR_COM;

 finish:
  switch (dev->descriptor.idProduct) {
  case PASORIUSB_PRODUCT_S310:
    pp->type = PASORI_TYPE_S310;
    break;
  case PASORIUSB_PRODUCT_S320:
    pp->type = PASORI_TYPE_S320;
    break;
  case PASORIUSB_PRODUCT_S330:
    pp->type = PASORI_TYPE_S330;
    break;
  default:
    return PASORI_ERR_TYPE;
  }

  pp->dh = usb_open(dev);
  pp->dev = dev;
  pp->timeout = TIMEOUT;
  get_end_points(pp);

  if (usb_set_configuration(pp->dh, 1)) {
    /* error */
    return PASORI_ERR_COM;
  }

  if (usb_claim_interface
      (pp->dh, pp->dev->config->interface->altsetting->bInterfaceNumber)) {
    /* error */
    return PASORI_ERR_COM;
  }
  return 0;
#endif	/* HAVE_LIBUSB_1 */
}

pasori *
pasori_open(void)
{
  pasori *pp;

  pp = (pasori *) malloc(sizeof(pasori));

  if (pp == NULL)
    return NULL;

  memset(pp, 0, sizeof(pasori));
  pp->i_ep_in = 0x81;

  if (open_usb(pp)) {
    pasori_close(pp);
    return NULL;
  }

  return pp;
}

int
pasori_send(pasori *pp, uint8 *data, int *size)
{
  uint8 resp[256];
  signed int i = - 1;
#ifdef HAVE_LIBUSB_1		/* HAVE_LIBUSB_1 */
  int length;
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
  int length;
#endif
  if (pp == NULL || data == NULL || size == NULL)
    return PASORI_ERR_PARM;

  if (*size < 1)
    return 0;

  Log("(send) send:");
  dbg_dump(data, *size);

  switch (pp->type) {
  case PASORI_TYPE_S310:
  case PASORI_TYPE_S320:
#ifdef HAVE_LIBUSB_1		/* HAVE_LIBUSB_1 */
    i = libusb_control_transfer(pp->dh, LIBUSB_REQUEST_TYPE_VENDOR, 0, 0, 0, data, *size, pp->timeout);
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
// TODO: support WebUSB
#else  /* HAVE_LIBUSB_1 */
    i = usb_control_msg(pp->dh, USB_TYPE_VENDOR, 0, 0, 0, data, *size, pp->timeout);
#endif	/* HAVE_LIBUSB_1 */
    break;
  case PASORI_TYPE_S330:
#ifdef HAVE_LIBUSB_1		/* HAVE_LIBUSB_1 */
    i = libusb_bulk_transfer(pp->dh, pp->b_ep_out, data, *size, &length, pp->timeout);
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
    i = webusb_rw_transfer_out(data, *size);
#else  /* HAVE_LIBUSB_1 */
    i = usb_bulk_write(pp->dh, pp->b_ep_out, data, *size, pp->timeout);
#endif	/* HAVE_LIBUSB_1 */
    break;
  default:
    return PASORI_ERR_TYPE;
  }

#if defined(HAVE_WEBUSB)
  // FIXME: inspect why removing this printf make polling functionality broken
  printf("%d", i);
#endif
  if (i < 0)
    return PASORI_ERR_COM;			/* FIXME:HANDLE INVALID RESPONSES */

#ifdef HAVE_LIBUSB_1		/* HAVE_LIBUSB_1 */
  *size = length;
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
  *size = length;
#else
  *size = i;
#endif

  switch (pp->type) {
  case PASORI_TYPE_S310:
  case PASORI_TYPE_S320:
#ifdef HAVE_LIBUSB_1		/* HAVE_LIBUSB_1 */
    i = libusb_interrupt_transfer(pp->dh, pp->i_ep_in, resp, sizeof(resp), &length, pp->timeout);
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
// TODO: support WebUSB
#else  /* HAVE_LIBUSB_1 */
    i = usb_interrupt_read(pp->dh, pp->i_ep_in, resp, sizeof(resp), pp->timeout);
#endif	/* HAVE_LIBUSB_1 */
    break;
  case PASORI_TYPE_S330:
#ifdef HAVE_LIBUSB_1		/* HAVE_LIBUSB_1 */
    i = libusb_bulk_transfer(pp->dh, pp->b_ep_in, resp, sizeof(resp), &length, pp->timeout);
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
    length = webusb_rw_transfer_in(resp, sizeof(resp));
#else  /* HAVE_LIBUSB_1 */
    i = usb_bulk_read(pp->dh, pp->b_ep_in, resp, sizeof(resp), pp->timeout);
#endif	/* HAVE_LIBUSB_1 */
    break;
  default:
    return PASORI_ERR_TYPE;
  }

#ifdef HAVE_LIBUSB_1		/* HAVE_LIBUSB_1 */
  if (i)
    return PASORI_ERR_COM;			/* FIXME:HANDLE INVALID RESPONSES */

  if (length != 6)
    return PASORI_ERR_DATA;

  i = length;
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
  if (length != 6)
    return PASORI_ERR_DATA;
#else
  if (i < 0)
    return PASORI_ERR_COM;			/* FIXME:HANDLE INVALID RESPONSES */

  if (i != 6)
    return PASORI_ERR_DATA;
#endif

  if (resp[4] != 0xff)
    return PASORI_ERR_DATA;

  /* debug */
  Log("(ACK?) recv:");
  dbg_dump(resp, i);

  return 0;

}

int
pasori_recv(pasori *pp, uint8 *data, int *size)
{
  signed int i;
#ifdef HAVE_LIBUSB_1		/* HAVE_LIBUSB_1 */
  int length;
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
  int length;
#endif	/* HAVE_LIBUSB_1 */

  if (pp == NULL || data == NULL || size == NULL)
    return 1;

  if (*size < 1)
    return 0;

  switch (pp->type) {
  case PASORI_TYPE_S310:
  case PASORI_TYPE_S320:
#ifdef HAVE_LIBUSB_1		/* HAVE_LIBUSB_1 */
    length = *size;
    i = libusb_interrupt_transfer(pp->dh, pp->i_ep_in, data, length, size, pp->timeout);
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
// TODO: support WebUSB
#else  /* HAVE_LIBUSB_1 */
    i = usb_interrupt_read(pp->dh, pp->i_ep_in, data, *size, pp->timeout);
#endif	/* HAVE_LIBUSB_1 */
    break;
  case PASORI_TYPE_S330:
#ifdef HAVE_LIBUSB_1		/* HAVE_LIBUSB_1 */
    length = *size;
    i = libusb_bulk_transfer(pp->dh, pp->b_ep_in, data, length, size, pp->timeout);
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
    length = webusb_rw_transfer_in(data, *size);
#else  /* HAVE_LIBUSB_1 */
    i = usb_bulk_read(pp->dh, pp->b_ep_in, data, *size, pp->timeout);
#endif
    break;
  default:
    return PASORI_ERR_TYPE;
  }

#ifdef HAVE_LIBUSB_1		/* HAVE_LIBUSB_1 */
  if (i) {
    Log("(recv) ERROR %d\n", i);
    return PASORI_ERR_COM;
  }
  i = length;
#elif defined(HAVE_WEBUSB)  /* HAVE_LIBUSB_1 */
  // TODO: support WebUSB
  // TODO: actually, need to handle inner exceptions.
#else
  if (i < 0) {
    Log("(recv) ERROR\n");
    return PASORI_ERR_COM;
  }
#endif

  Log("(recv) recv:");
  dbg_dump(data, i);
  *size = i;

  return 0;
}
