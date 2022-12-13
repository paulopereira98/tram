#!/bin/bash


PTP_TESTS_DIR=../../tests/1-ptp/

python3 ptp_plotter.py \
    -f ${PTP_TESTS_DIR}/1-direct-ptp-hw-linreg.log       -l hw-linreg        \
    -f ${PTP_TESTS_DIR}/1-direct_ptp_sw_linreg_hwm.log   -l sw_linreg_hwm    \
    -f ${PTP_TESTS_DIR}/1-direct_ptp_sw_pi_hwm.log       -l sw_pi_hwm        \
    -f ${PTP_TESTS_DIR}/1-direct-ptp-hw-pi.log           -l hw-pi            \
    -f ${PTP_TESTS_DIR}/1-direct_ptp_sw_linreg_swm.log   -l sw_linreg_swm    \
    -f ${PTP_TESTS_DIR}/1-direct_ptp_sw_pi_swm.log       -l sw_pi_swm        \
    $@
