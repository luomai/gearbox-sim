# !bin/bash
cd /home/lm111/13-LADIS/src/sim
mkdir sim_results/vary_std

for i in {1..7}; do
	mkdir sim_results/vary_std/$i
	for j in 0.05 0.07 0.09 0.11 0.13 0.15; do
		mkdir sim_results/vary_std/$i/$j
		cat sim_config/config_file_template | sed 's/token_perf_std:[0-9]*.[0-9]*/token_perf_std:'$j'/' > sim_config/config_file_tmp
		cat sim_config/config_file_tmp | sed 's/policy_id:[0-9]*/policy_id:'$i'/' > sim_config/config_file
		./sim -u Cmdenv
		cp sim_config/config_file sim_results/vary_std/$i/$j/
		cp results_file sim_results/vary_std/$i/$j/
		cp -r results sim_results/vary_std/$i/$j/ 
	done
done
