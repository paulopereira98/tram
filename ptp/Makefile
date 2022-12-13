
.PHONY: clean install install_dependencies install_python_dependencies


install:
	echo -n "NTP must is disabled not to interfere with PTP"
	#sudo systemctl status systemd-timesyncd.service
	timedatectl set-ntp false

install_dependencies: install_python_dependencies
	#

install_python_dependencies:
	sudo apt install -y pip
	pip install matplotlib
