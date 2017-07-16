name=${1}
st=${2}
ed=${3}
for i in `seq ${st} ${ed}`;
do
  ./self_match.sh ${name} ${i} $((42485 + ${i}))
done
