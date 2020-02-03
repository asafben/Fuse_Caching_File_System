# MOUNTDIR=/home/user/os_ex4/dirs/mountdir
# ROOTDIR=/home/user/os_ex4/dirs/rootdir
MOUNTDIR=/tmp/mountdir
ROOTDIR=/tmp/rootdir
CNT=1
# CachingFileSystem rootdir mountdir numberOfBlocks fOld fNew


# caching: CachingFileSystem.cpp naive.cpp naive.h logger.h logger.cpp
# 	g++ -Wall CachingFileSystem.cpp naive.cpp naive.h logger.h logger.cpp `pkg-config fuse --cflags --libs` -o caching
# cmp -s mountdir1/RandFile rootdir/RandFile

CachingFileSystem: CachingFileSystem.cpp
	g++ -std=c++11 -Wall CachingFileSystem.cpp `pkg-config fuse --cflags --libs` -o CachingFileSystem

test_fuse: fuse_unit_tests.cpp
	g++ -std=c++11 fuse_unit_tests.cpp `pkg-config fuse --cflags --libs` -o test_fuse

testf:test_fuse
	./test_fuse

run: CachingFileSystem
	./CachingFileSystem $(ROOTDIR) $(MOUNTDIR)$(CNT) 500 0.3 0.7 

all:setup CachingFileSystem test_fuse
	chmod +x CachingFileSystem
	chmod +x test_fuse
	chmod +x fuseTest.py

kill:
	fusermount -u $(MOUNTDIR)$(CNT)

bnaia:
	python fuseTest.py ./CachingFileSystem

noam:
	python3 test.py

noam1:
	python3 testmicro.py

ioctl:
	python -c "import os,fcntl; fd = os.open('$(MOUNTDIR)$(CNT)/1', os.O_RDONLY); fcntl.ioctl(fd, 0); os.close(fd)"

setup:
	mkdir /tmp/mountdir$(CNT)
	mkdir /tmp/rootdir
	mkdir /tmp/rootdir/f
	mkdir ./dirs
	touch ./dirs/small
	echo "1" > ./dirs/small
	touch ./dirs/nums
	echo "0123456789" > ./dirs/nums
	touch /tmp/rootdir/1
	touch /tmp/rootdir/2
	for i in {1..3}; do echo "0123456789" >> /tmp/rootdir/1; done
	for i in {1..2000}; do echo "0123456789" >> /tmp/rootdir/2; done
	dd if=/dev/urandom of=/tmp/rootdir/RandFile bs=4096 count=1500
	dd if=/dev/urandom of=/tmp/rootdir/RandFile2 bs=4000 count=3
	dd if=/dev/urandom of=/tmp/rootdir/RandFile3 bs=4096 count=8


push:
	git add -A
	git commit -m "generic push"
	git push -u origin master
pull:
	git pull

clean:
	rm -rf caching CachingFileSystem test_fuse test_naive ./dirs/small /tmp/rootdir/RandFile ./dirs/nums /tmp/rootdir/1 /tmp/rootdir/2 /tmp/mountdir$(CNT) /tmp/rootdir ./dirs zzz_backup_log zzz_debug_log
 
tar:
	tar cfv ex4.tar CachingFileSystem.cpp Makefile README
