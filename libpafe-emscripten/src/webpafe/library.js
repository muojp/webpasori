//"use strict";

var mylib = {
    my_js__deps: ['$EmterpreterAsync'],
    my_js: function () {
        return EmterpreterAsync.handle(function (resume) {
            alert('hi');
        });
    },
};

mergeInto(LibraryManager.library, mylib);


var webpasori = {
    usbDevice: undefined,
    epNumber: -1,
    webpasori_openusb__deps: ['$EmterpreterAsync'],
    webpasori_openusb: function (vendorId, productIdS310, productIdS320, productIdS330) {
        var PASORIUSB_VENDOR = vendorId;
        var PASORIUSB_PRODUCT_S310 = productIdS310;
        var PASORIUSB_PRODUCT_S320 = productIdS320;
        var PASORIUSB_PRODUCT_S330 = productIdS330;

        return EmterpreterAsync.handle(function (resume) {
            var devices = navigator.usb.getDevices()
                .then(function(devices) {
                    devices.some(function(dev) {
                        if (dev.productId === PASORIUSB_PRODUCT_S310
                            || dev.productId === PASORIUSB_PRODUCT_S320
                            || dev.productId === PASORIUSB_PRODUCT_S330) {
                            this.usbDevice = dev;
                            return true;
                        }
                    });
                    console.log('device:');
                    console.log(this.usbDevice);
                    if (this.usbDevice !== undefined) {
                        this.usbDevice.open()
                        .then(function() {
                            resume(function() { return 0; });
                        });
                    }
                    else {
                        // permission granted but device not found
                        resume(function() { return -1; });
                    }
                })
                .catch(function() {
                    // (maybe) permission not granted
                    resume(function() { return -2; });
                })
        });
    },
    webpasori_get_reader_type: function() {
        if (this.usbDevice === undefined) {
            return -1;
        }
        return this.usbDevice.productId;
    },
    webpasori_closeusb: function() {
        if (this.usbDevice === undefined) {
            return -1;
        }
        return EmterpreterAsync.handle(function (resume) {
            this.usbDevice.close()
            .then(function() {
                resume(function() { return 0; });
            });
        });
    },
    webpasori_get_endpoints__deps: ['$EmterpreterAsync'],
    webpasori_get_endpoints: function() {
        if (this.usbDevice === undefined) {
            return -1;
        }
        // (re)initialize endpoint Number
        this.epNumber = -1;
        this.usbDevice.configuration.interfaces.forEach(function(iface) {
            iface.alternates.forEach(function(alt) {
                alt.endpoints.forEach(function(ep) {
                    if (ep.type === 'bulk' && ep.direction === 'in') {
                        this.epNumber = ep.endpointNumber;
                        console.log('found bulk-in');
                    }
                    else if (ep.type === 'bulk' && ep.direction === 'out') {
                        // do nothing. 'bulk' && 'in' is just enough
                        console.log('found bulk-out');
                    }
                    else if (ep.type === 'interrupt' && ep.direction === 'in') {
                        console.log('found interrupt-in');
                    }
                });
            });
        });
        return this.epNumber === -1 ? -1 : 0;
    },
    webpasori_claim_interface__deps: ['$EmterpreterAsync'],
    webpasori_claim_interface: function(num) {
        if (this.usbDevice === undefined) {
            return -1;
        }
        return EmterpreterAsync.handle(function (resume) {
            this.usbDevice.claimInterface(num)
            .then(function() {
                console.log('USB interface claimed.');
                resume(function() { return 0; });
            })
            .catch(function() {
                resume(function() { return -2; });
            });
        });
    },
    webusb_rw_transfer_out__deps: ['$EmterpreterAsync'],
    webusb_rw_transfer_out: function(data, size) {
        if (this.usbDevice === undefined || this.epNumber === -1) {
            return -1;
        }
        var buf = new ArrayBuffer(size);
        var arr = new Uint8Array(buf, 0, buf.length);
        for (var cnt = 0; cnt !== size; ++cnt) {
            arr[cnt] = getValue(data + cnt, 'i8');
        }
        // assume all bytes are written successfully
        return EmterpreterAsync.handle(function (resume) {
            this.usbDevice.transferOut(this.epNumber, arr)
            .then(resume(function() { return size; }));
        });
    },
    webusb_rw_transfer_in__deps: ['$EmterpreterAsync'],
    webusb_rw_transfer_in: function(data, size) {
        if (this.usbDevice === undefined || this.epNumber === -1) {
            return -1;
        }
        return EmterpreterAsync.handle(function (resume) {
            this.usbDevice.transferIn(this.epNumber, size)
            .then(function(ret) {
                for (var cnt = 0; cnt !== ret.data.byteLength; ++cnt) {
                    setValue(data + cnt, ret.data.getInt8(cnt), 'i8');
                }
                resume(function() { return ret.data.byteLength; });
            });
        });
    }
};

mergeInto(LibraryManager.library, webpasori);
