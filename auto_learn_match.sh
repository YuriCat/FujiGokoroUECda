# ビルド, 学習, 方策プレーでの対戦実験, モンテカルロでの対戦実験を一気にやる
# 設定
time=`date '+%s'`
echo ${time}

# linux server
auto_dir=/home/ohto/data/uecda/auto/
learn_record_dir=/home/ohto/data_research/uecda/log/
opponent_dir=/home/ohto/ohto/games/uecda/linux/clients/
current_dir=/home/ohto/ohto/projects/uecda/

# mac laptop
#auto_dir=/Users/ohto/documents/data/uecda/auto/
#learn_record_dir=/Users/ohto/documents/data_research/uecda/log/
#opponent_dir=/Users/ohto/dropbox/ohto/games/uecda/mac/clients/
#current_dir=/Users/ohto/dropbox/ohto/projects/uecda/


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
./out/release/pglearner -o ${save_param_dir} -t -i 200\
 -l ${learn_record_dir}gli161023_30000_4.dat\
 -l ${learn_record_dir}glix5_10000.dat\
 -l ${learn_record_dir}gli161023_expac.dat\
 -l ${learn_record_dir}gl160116_beer.dat\
 -l ${learn_record_dir}gl160116_e10.dat\
 -l ${learn_record_dir}gl160116_fj.dat\
 -l ${learn_record_dir}gl160116_pao.dat\
 -l ${learn_record_dir}wisteriax5_50000_9.dat\
 -l ${learn_record_dir}wisteriax5_50000_10.dat\
 -l ${learn_record_dir}wisteriax5_50000_11.dat\
 -l ${learn_record_dir}wisteriax5_50000_12.dat\
 -l ${learn_record_dir}gli_att_wis_3000.dat\
 -l ${learn_record_dir}gli_att_wis_1000.dat\
 -l ${learn_record_dir}gli_thres_wis_2000.dat\
 -l ${learn_record_dir}wisteriax5_15000.dat\
 -l ${learn_record_dir}wisteriax5_50000_5.dat\
 -l ${learn_record_dir}wisteriax5_50000_6.dat\
 -l ${learn_record_dir}wisteriax5_50000_7.dat\
 -l ${learn_record_dir}wisteriax5_50000_8.dat\
 -l ${learn_record_dir}gl160116_wis_0.dat\
 -l ${learn_record_dir}gl160116_wis_1.dat\
 -l ${learn_record_dir}gl160116_wis_2.dat\
 -l ${learn_record_dir}gl160116_wis_3.dat\
 -l ${learn_record_dir}wis_gli161019_tempexp.dat\
 -l ${learn_record_dir}wis_gli161018.dat\
 -l ${learn_record_dir}epoch6_mc5000_old0.dat\
 -l ${learn_record_dir}epoch6_mc5000_old1.dat\
 -l ${learn_record_dir}epoch6_mc5000_old2.dat\
 -l ${learn_record_dir}wisteriax5_50000_1.dat\
 -l ${learn_record_dir}wisteriax5_50000_2.dat\
 -l ${learn_record_dir}wisteriax5_50000_3.dat\
 -l ${learn_record_dir}wisteriax5_50000_4.dat\
 -l ${learn_record_dir}epoch3_mc5000_0.dat\
 -l ${learn_record_dir}epoch3_mc5000_1.dat\
 -l ${learn_record_dir}epoch3_mc5000_2.dat\
 -l ${learn_record_dir}epoch3_mc5000_3.dat\
 -l ${learn_record_dir}epoch4_mc5000_0.dat\
 -l ${learn_record_dir}epoch4_mc5000_1.dat\
 -l ${learn_record_dir}epoch4_mc5000_2.dat\
 -l ${learn_record_dir}epoch4_mc5000_3.dat\
 -l ${learn_record_dir}epoch5_mc5000_0.dat\
 -l ${learn_record_dir}epoch5_mc5000_1.dat\
 -l ${learn_record_dir}epoch5_mc5000_2.dat\
 -l ${learn_record_dir}epoch5_mc5000_3.dat\
 -l ${learn_record_dir}epoch6_mc5000_0.dat\
 -l ${learn_record_dir}epoch6_mc5000_1.dat\
 -l ${learn_record_dir}epoch6_mc5000_2.dat\
 -l ${learn_record_dir}epoch6_mc5000_3.dat\
 -l ${learn_record_dir}epoch7_mc5000_0.dat\
 -l ${learn_record_dir}epoch7_mc5000_1.dat\
 -l ${learn_record_dir}epoch7_mc5000_2.dat\
 -l ${learn_record_dir}epoch7_mc5000_3.dat\
 -l ${learn_record_dir}epoch8_mc5000_0.dat\
 -l ${learn_record_dir}epoch8_mc5000_1.dat\
 -l ${learn_record_dir}epoch8_mc5000_2.dat\
 -l ${learn_record_dir}epoch8_mc5000_3.dat\
 -l ${learn_record_dir}epoch9_mc5000_0.dat\
 -l ${learn_record_dir}epoch9_mc5000_1.dat\
 -l ${learn_record_dir}epoch9_mc5000_2.dat\
 -l ${learn_record_dir}epoch9_mc5000_3.dat\
 -l ${learn_record_dir}wisteriax5_50000.dat\
 -l ${learn_record_dir}FujiGokorox5_50000.dat\
 -l ${learn_record_dir}paoonx5_50000.dat\
 2>${save_dir}policy_learn_log.txt

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

