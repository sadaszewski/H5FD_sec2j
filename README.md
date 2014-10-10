H5FD_sec2j
==========

H5FD_sec2j is a journalling filesystem driver for HDF5 based on SEC2 driver

The architecture od SEC2J is as follows:

A journal file format:

```
Header:

|------------------------------|-----------------------------|
| Magic string SEC2J (5 bytes) | Last entry offset (8 bytes) |
|------------------------------|-----------------------------|

Followed by entries of the following format:

|-------------------|----------------|-----------------|-
| Address (8 bytes) | Size (8 bytes) | CRC32 (4 bytes) |
|-------------------|----------------|-----------------|-

---------------------|---------------------------------|
Data (variable size) | Previous entry offset (8 bytes) |
---------------------|---------------------------------|

```

Two new functions are introduced:

```C
herr_t H5FD_sec2j_tx_start(hid_t __f1);
herr_t H5FD_sec2j_tx_end(hid_t __f1);
```

to respectively start and finish a journalled transaction on the given HDF5 file descriptor.

If the transaction gets interrupted by a process crash, upon opening the file next time the journaled changes will be reverted bringing the file to the state before transaction.

Consistency is assured by calling fsync() for both underlying SEC2 file and the journal file to ensure the journal never gets disposed before HDF5 file state is stable.
