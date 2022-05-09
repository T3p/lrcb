#!/bin/zsh

TODAY=$(date +'%Y-%m-%d')
HORIZON=200000
SEEDS=1075484,9162318,443978,3634026,3793308,6909422,12060294,9781979,6422775,6889559,4433229,9137294,11142955,11622665,11071819,10685008,1826234,10255678,5845421,4973696,2562043,11068055,11403227,797011,5420824,4210547,7220797,7996068,10793697,1545028,6303414,6333329,4436474,2038012,1051209,10629003,2522454,583943,9626970,7035112,7188061,11119344,3104059,8223280,10024927,2040284,11522911,5513714,9495998,7998997,3310829,6532636,2486406,8807646,7526380,8077982,3844517,8696743,1616510,3285393,2120292,7944330,10395455,437858,7476825,9837730,7105685,11992484,10263264,11554104,1566006,811589,12241346,11799123,4538090,3065893,8773058,427718,21299,9743312,11190030,12309429,2464799,7924813,5327167,3976309,7259068,7668681,359588,11797641,9815856,2351297,7508864,5922119,10086756,5181573,11312072,1644975,7734936,9581601
for DOMAIN in categorical_c2
do
    python new_lbrl_runner.py --multirun horizon=${HORIZON} domain=${DOMAIN}.yaml algo=srllinucb_mineig_norm,srllinucb_avg_quad_norm,srlegreedy_mineig_norm,srlegreedy_avg_quad_norm,leader seed=${SEEDS} hydra.sweep.dir=${DOMAIN}_${TODAY}/\${algo} use_wandb=false use_tb=true eps_decay=sqrt
    # python new_lbrl_runner.py --multirun horizon=${HORIZON} domain=${DOMAIN}.yaml algo=linucb,egreedyglrt rep_idx=0,1,2,3,4,5 seed=${SEEDS} hydra.sweep.dir=${DOMAIN}_${TODAY}/\${algo}_\${rep_idx} use_wandb=false use_tb=true eps_decay=sqrt
done
