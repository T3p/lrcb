# Learning Representations for Contextual Bandits


Do not look into tmp_code

# How to Replicate the Experiments

For the experiments in the main paper, you can run the following commands

### wheel

    python run_linearfinite.py -m domain=wheel.yaml horizon=200000 algo=nnlinucb check_glrt=True epsilon_decay=none bonus_scale=3 glrt_scale=5 layers=\"100,100,50,20,10\" weight_mse=1 weight_rayleigh=0 weight_min_features=0,1 weight_min_random=0 weight_l2features=0 weight_trace=0 weight_spectral=0 seed=713,464,777,879,660,608,773,919,591,229 use_tb=true use_maxnorm=False hydra.sweep.dir=expmain/wheel/ hydra.sweep.subdir=\${algo}_weak\${weight_min_features}_\${seed} hydra.launcher.submitit_folder=expmain/wheel/.slurm


    python run_linearfinite.py -m domain=wheel.yaml horizon=200000 algo=nnegreedy check_glrt=True epsilon_decay=cbrt bonus_scale=3 glrt_scale=5 layers=\"100,100,50,20,10\" weight_mse=1 weight_rayleigh=0 weight_min_features=0,1 weight_min_random=0 weight_l2features=0 weight_trace=0 weight_spectral=0 seed=713,464,777,879,660,608,773,919,591,229 use_tb=true use_maxnorm=False hydra.sweep.dir=expmain/wheel/ hydra.sweep.subdir=\${algo}_weak\${weight_min_features}_\${seed} hydra.launcher.submitit_folder=expmain/wheel/.slurm