all:
	echo "ProjetoB"
	gcc -Wall -o partB.exe disk.c ppos_disk.c ppos-core-aux.c pingpong-disco1.c libppos_static.a -lrt
	gcc -Wall -o partB2.exe disk.c ppos_disk.c ppos-core-aux.c pingpong-disco2.c libppos_static.a -lrt

pA:
	echo "ProjetoA"
	gcc -Wall -o pingpong_scheduler.exe ppos-core-aux.c pingpong-scheduler.c libppos_static.a
	gcc -Wall -o pingpong_preemp.exe ppos-core-aux.c pingpong-preempcao.c libppos_static.a
	gcc -Wall -o pingpong_contab_prio.exe ppos-core-aux.c pingpong-contab-prio.c libppos_static.a

pB:
	echo "ProjetoB"
	gcc -Wall -o out.exe disk.c ppos_disk.c pingpong-disco1.c ppos-core-aux.c libppos_static.a -lrt