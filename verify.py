from sec2j import sec2j
from multiprocessing import Process, Queue
import os
import time
import numpy as np
import h5py
import sys
import random


def write_h5file(q):
	# print 'Here'
	fname = '/home/adaszews/mnt/mouse_brain/scratch/test.h5'
	if os.path.exists(fname):
		os.unlink(fname)
	if os.path.exists(fname + '.journal'):
		os.unlink(fname + '.journal')
	f = sec2j.open(fname)
	# print 'Here 2'
	cnt = 0
	while True:
		try:
			a = q.get(False)
			print 'sec2j.set_exit()'
			sec2j.set_exit(-1)
		except:
			pass

		print '*',
		sys.stdout.flush()
		sec2j.tx_start(f.id.id)
		g = f.require_group('dupa/%d' % cnt)
		d = g.create_dataset('data', shape=(100, 100, 100), dtype=np.uint8)
		d[:, :, :] = np.random.random([100, 100, 100])
		cnt += 1
		print 'Finishing'
		sec2j.tx_end(f.id.id)
	f.flush()
	f.close()


def main():
	print 'Testing different kill times'
	n = 3
	histo = [0] * n
	for i in range(2, n):
		print 'i:', i
		for k in range(0, 1):
			q = Queue()
			p = Process(target=write_h5file, args=(q,))
			p.start()
			# time.sleep(0.05 * i)
			q.put(1)
			p.join()
			try:
				f = h5py.File('test.h5', 'r')
				f.close()
			except:
				print 'Broken'
				histo[i] += 1
	print histo


if __name__ == '__main__':
	main()
