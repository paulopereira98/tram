#!/bin/bash


PLOTTER_DIR=${0%/*}
PTP_TESTS_DIR=$PLOTTER_DIR/../../tests/2-ptp/

ALL=" echo '' "
group_by_controller=" echo '' "
group_by_usecase=" echo '' "

# All plots
if false
then
    ALL="\
    python3 $PLOTTER_DIR/ptp_plotter.py \
        -f ${PTP_TESTS_DIR}/2-direct-ptp-hw-linreg.log       -l hw-linreg        \
        -f ${PTP_TESTS_DIR}/2-direct_ptp_sw_linreg_hwm.log   -l sw_linreg_hwm    \
        -f ${PTP_TESTS_DIR}/2-direct_ptp_sw_linreg_swm.log   -l sw_linreg_swm    \
        -f ${PTP_TESTS_DIR}/2-direct-ptp-hw-pi.log           -l hw-pi            \
        -f ${PTP_TESTS_DIR}/2-direct_ptp_sw_pi_hwm.log       -l sw_pi_hwm        \
        -f ${PTP_TESTS_DIR}/2-direct_ptp_sw_pi_swm.log       -l sw_pi_swm        \
        $@"
fi


# Group by controller
if true
then
    group_by_controller="\
    python3 $PLOTTER_DIR/ptp_plotter.py \
        -f ${PTP_TESTS_DIR}/2-direct-ptp-hw-linreg.log       -l hw-linreg        \
        -f ${PTP_TESTS_DIR}/2-direct_ptp_sw_linreg_hwm.log   -l sw_linreg_hwm    \
        -f ${PTP_TESTS_DIR}/2-direct_ptp_sw_linreg_swm.log   -l sw_linreg_swm    \
        $@\
    &\
    python3 $PLOTTER_DIR/ptp_plotter.py \
        -f ${PTP_TESTS_DIR}/2-direct-ptp-hw-pi.log           -l hw-pi            \
        -f ${PTP_TESTS_DIR}/2-direct_ptp_sw_pi_hwm.log       -l sw_pi_hwm        \
        -f ${PTP_TESTS_DIR}/2-direct_ptp_sw_pi_swm.log       -l sw_pi_swm        \
        $@"
fi


# Group by Usecase
if true
then
    group_by_usecase="\
    python3 $PLOTTER_DIR/ptp_plotter.py \
        -f ${PTP_TESTS_DIR}/2-direct-ptp-hw-linreg.log       -l hw-linreg        \
        -f ${PTP_TESTS_DIR}/2-direct-ptp-hw-pi.log           -l hw-pi            \
        $@\
    &\
    python3 $PLOTTER_DIR/ptp_plotter.py \
        -f ${PTP_TESTS_DIR}/2-direct_ptp_sw_linreg_hwm.log   -l sw_linreg_hwm    \
        -f ${PTP_TESTS_DIR}/2-direct_ptp_sw_pi_hwm.log       -l sw_pi_hwm        \
        $@\
    &\
    python3 $PLOTTER_DIR/ptp_plotter.py \
        -f ${PTP_TESTS_DIR}/2-direct_ptp_sw_linreg_swm.log   -l sw_linreg_swm    \
        -f ${PTP_TESTS_DIR}/2-direct_ptp_sw_pi_swm.log       -l sw_pi_swm        \
        $@"
fi

eval "$ALL & $group_by_controller & ${group_by_usecase}"