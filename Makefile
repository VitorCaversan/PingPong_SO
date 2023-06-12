all:
	echo "ProjetoA"
	gcc -o pingpong_scheduler.exe ppos-core-aux.c pingpong-scheduler.c libppos_static.a
	gcc -o pingpong_preemp.exe ppos-core-aux.c pingpong-preempcao.c libppos_static.a
	gcc -o pingpong_contab_prio.exe ppos-core-aux.c pingpong-contab-prio.c libppos_static.a
	echo "ProjetoB"
	gcc -o out.exe disk.c ppos_disk.c ppos-core-aux.c pingpong-disco1.c libppos_static.a -lrt

pA:
	echo "ProjetoA"
	gcc -o pingpong_scheduler.exe ppos-core-aux.c pingpong-scheduler.c libppos_static.a
	gcc -o pingpong_preemp.exe ppos-core-aux.c pingpong-preempcao.c libppos_static.a
	gcc -o pingpong_contab_prio.exe ppos-core-aux.c pingpong-contab-prio.c libppos_static.a

pB:
	echo "ProjetoB"
	gcc -o out.exe pingpong-disco1.c ppos_disk.c disk.c ppos-core-aux.c libppos_static.a -lrt