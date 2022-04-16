(qemu-arm $1)
c=$?

if [ "$2" != "-quiet" ]; then
	echo Process exited with exit code ${c}
fi

exit ${c}
