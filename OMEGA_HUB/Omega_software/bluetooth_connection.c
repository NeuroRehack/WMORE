#This is the bluetooth connection script. Aimed at establising a conneciton
#with the Loggers/Coordinators using Bluetooth Low Energy

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <bluetooth/uuid.h>
#include <bluetooth/l2cap.h>
