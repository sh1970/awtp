#!/bin/sh

acResult="/tmp/cloudAC"
serial=`uci get system.@system[0].hostname`
cloudURL="https://fmsweb.shixunet.com/agent-wx/cloudac/getAcByAp?apMac=$serial"
#cloudURL="http://1.1.1.1/agent-wx/cloudac/getAcByAp?apMac=$serial"
#cloudURL="https://fmsweb.shixunet.com/agent-wx/cloudac/getAcByAp?apMac=44:d1:fa:e3:eb:df"
/usr/bin/wget -T 60 -q -O $acResult $cloudURL
#{"code":0,"data":{"acID":"ptb6ca888c3ee3wiosq","acIP":"210.14.142.89","acMAC":"B6-CA-88-8C-3E-E3","acPort":"15202"},"msg":"success"}

if [ -f "$acResult" ]; then
        cloudAC=`cat $acResult`
        if [[ "$cloudAC" == *"success"* ]]; then
                tmpIP=${cloudAC#*\"acIP\":\"}                                                                                         
                acIP=${tmpIP%%\",\"acMAC\"*}    
                tmpPort=${cloudAC#*\"acPort\":\"}
                acPort=${tmpPort%%\"\},\"*}                                                          
                /bin/sed -i "/option server/c\ \toption server '$acIP'" /etc/config-shadow/ucentral
                /bin/sed -i "/option port/c\ \toption port '$acPort'" /etc/config-shadow/ucentral
                reload_config
                sleep 60
		/usr/bin/killall -9 flock           
		sleep 3                
		/sbin/reboot
	fi                  
	/bin/rm -f $acResult
fi
