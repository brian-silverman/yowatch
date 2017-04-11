#xxd -r -p $1 | dd conv=swab > $1.pcm
xxd -r -p $1 > $1.pcm
sh pcmtowav.sh $1


