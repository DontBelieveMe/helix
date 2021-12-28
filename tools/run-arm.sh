c=$(qemu-arm $1)$?

if [ "$2" != "-quiet" ]; then
	echo Process exited with exit code ${c}
fi

exit ${c}
