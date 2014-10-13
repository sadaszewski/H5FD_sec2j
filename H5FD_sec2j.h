//
// Journalling support for HDF5 sec2 file driver
//
// Author: Stanislaw Adaszewski, 2014
// E-mail: s.adaszewski@gmail.com
// http://algoholic.eu
//

#ifndef _H5FD_SEC2J__H_
#define _H5FD_SEC2J__H_

#include <H5Ipublic.h>

#define H5FD_SEC2J	(H5FD_sec2j_init())

#ifdef __cplusplus
extern "C" {
#endif

H5_DLL hid_t H5FD_sec2j_init();
H5_DLL void H5FD_sec2j_term();
H5_DLL herr_t H5Pset_fapl_sec2j(hid_t fapl_id);

H5_DLL herr_t H5FD_sec2j_tx_start(hid_t __f1);
H5_DLL herr_t H5FD_sec2j_tx_end(hid_t __f1);

H5_DLL void H5FD_sec2j_set_exit(int);

#ifdef __cplusplus
}
#endif

#endif // _H5FD_SEC2J__H_
