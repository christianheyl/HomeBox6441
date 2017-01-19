#!/bin/sh /etc/rc.common
START=79

start() {
        if [ A"$CONFIG_FEATURE_LQ_COC_APP_PMD_AUTO_START_D1" = "A1" ]; then
            echo "start Power Management Daemon with D1"
            /opt/lantiq/bin/pmcu_utility -r1&
            /opt/lantiq/bin/pmd 20 60 85 200 20 30 0 D1&
        fi
        if [ A"$CONFIG_FEATURE_LQ_COC_APP_PMD_AUTO_START_D2" = "A1" ]; then
            echo "start Power Management Daemon with D2"
            /opt/lantiq/bin/pmcu_utility -r1&
            /opt/lantiq/bin/pmd 20 60 85 200 20 30 0 D2&
        fi
        if [ A"$CONFIG_FEATURE_LQ_COC_APP_PMD_AUTO_START_D3" = "A1" ]; then
            echo "start Power Management Daemon with D3"
            /opt/lantiq/bin/pmcu_utility -r1&
            /opt/lantiq/bin/pmd 20 60 85 200 20 30 0 D3&
        fi
}
