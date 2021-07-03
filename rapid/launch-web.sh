SCEN=$1
#1 = cubic - BBR
#2 = cubic - cubic
#3 = BBR   - cubic

if [ $SCEN -eq 1 ]
then 
/home/mdiarra/bbr/proxy/start-web.sh web cubic+bbr 1,2,3,4,5 5 0 2 10 1
/home/mdiarra/bbr/proxy/start-web.sh web cubic+bbr 1,2,3,4,5 5 0 2 10 2
/home/mdiarra/bbr/proxy/start-web.sh web cubic+bbr 1,2,3,4,5 5 0 2 10 10
elif [ $SCEN -eq 2 ]
then
/home/mdiarra/bbr/proxy/webcc/start-web2.sh web cubic+bbr 1,2,3,4,5 5 0 2 10 1
/home/mdiarra/bbr/proxy/webcc/start-web2.sh web cubic+bbr 1,2,3,4,5 5 0 2 10 2
/home/mdiarra/bbr/proxy/webcc/start-web2.sh web cubic+bbr 1,2,3,4,5 5 0 2 10 10
else
/home/mdiarra/bbr/proxy/webbc/start-webb.sh web cubic+bbr 1,2,3,4,5 5 0 2 10 1
/home/mdiarra/bbr/proxy/webbc/start-webb.sh web cubic+bbr 1,2,3,4,5 5 0 2 10 2
/home/mdiarra/bbr/proxy/webbc/start-webb.sh web cubic+bbr 1,2,3,4,5 5 0 2 10 10
fi


#/home/mdiarra/bbr/legacy/start-mix.sh mix cubic+bbr 1,2,3,4 120 200 2 1 2
#/home/mdiarra/bbr/legacy/start-mix.sh mix cubic+bbr 1,2,3,4 120 200 2 1 50

#/home/mdiarra/WCNC2020-ProSCH_ns3/start-mix.sh pepem2 mix 1,2,3,4 120 200 2 10
#/home/mdiarra/WCNC2020-ProSCH_ns3/start-mix.sh pepem4 mix 1,2,3,4 120 200 4 10
