#!/bin/sh

case "${ACTION}" in
add|"")
	insmod /root/final_char_driver.ko
	;;
remove)
	rmmod final_char_driver
	;;
esac
