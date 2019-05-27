time=`date '+%s'`
echo ${time}

# 設定
auto_dir=
learn_record_dir=

# 保存ディレクトリ作成
save_dir=${auto_dir}${time}/
mkdir -p ${save_dir}

backup_dir=${save_dir}backup/
save_param_dir=${save_dir}param/
mkdir -p ${backup_dir}
mkdir -p ${save_param_dir}

# ビルド
make -j4

# データのバックアップを取る
zip -r ${backup_dir}uecda.zip .

# 学習
./out/learner -o ${save_param_dir} -th 8 -t -i 1000 -ld ${learn_record_dir} 2>${save_dir}policy_learn_log.txt
