time=`date '+%s'`
echo ${time}

# 設定
auto_dir=
learn_record_dir=
opponent_dir=
current_dir=

cd ..

# 保存ディレクトリ作成
save_dir=${auto_dir}${time}/
mkdir -p ${save_dir}

backup_dir=${save_dir}backup/
save_param_dir=${save_dir}param/
mkdir -p ${backup_dir}
mkdir -p ${save_param_dir}

# ビルド
make release -j4

# データのバックアップを取る
zip -r ${backup_dir}uecda.zip .

# 学習
./out/release/pglearner -o ${save_param_dir} -t -i 200 -ld ${learn_record_dir} 2>${save_dir}policy_learn_log.txt

# 方策オンリーでの対戦実験
./out/release/server -g 100000 1>${save_dir}policy_game.txt &
sleep 1.0
./out/release/policy_client -i ${save_param_dir} 2>${save_dir}policy_client_output.txt &

cd ${opponent_dir}

sleep 0.5
./kou2 &
sleep 0.5
./kou2 &
sleep 0.5
./kou2 &
sleep 0.5
./kou2 &

cd ${current_dir}

# モンテカルロでの対戦実験

cd scripts