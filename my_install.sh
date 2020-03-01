#See `sushi -h` for a complete list of options.
#Test in offline mode with I/O from audio file:
#    $ sushi -o -i input_file.wav -c config_file.json
#Use JACK for realtime audio:
#    $ sushi -j -c config_file.json
#sudo apt-get install libalsaplayer-dev libalsaplayer0 libsndfile1 libsndfile1-dev jackd2
#Raspa for elk board w cross compiling
#./generate --cmake-args="-DWITH_XENOMAI=off -DVST2_SDK_PATH=/home/fausto/elk/vstsdk2.4" -b
#./generate --cmake-args="-DWITH_XENOMAI=off -DWITH_VST2=off" -b
#find ~/elk/sushi \( -type d -name .h -prune \) -o -type f -print0 | xargs -0 sed -i 's/<google\/protobuf/<\/home\/fausto\/elk\/grpc\/third_party\/protobuf\/src\/google\/protobuf/g'
find ~/elk/sushi \( -type d -name .h -prune \) -o -type f -print0 | xargs -0 sed -i 's/<grpcpp\/impl\/codegen/<\/home\/fausto\/elk\/grpc\/include\/grpcpp\/impl\/codegen/g'
find ~/elk/grpc/include/grpc/impl/codegen/ \( -type d -name .h -prune \) -o -type f -print0 | xargs -0 sed -i 's/<grpcpp\/impl\/codegen/<\/home\/fausto\/elk\/grpc\/include\/grpcpp\/impl\/codegen/g'
find ~/elk/grpc/include/grpcpp/impl/codegen/ \( -type d -name .h -prune \) -o -type f -print0 | xargs -0 sed -i 's/<grpcpp\/impl\/codegen/<\/home\/fausto\/elk\/grpc\/include\/grpcpp\/impl\/codegen/g'
./generate --cmake-args="-DWITH_XENOMAI=off -DVST2_SDK_PATH=/home/fausto/elk/vstsdk2.4 -DWITH_RPC_INTERFACE=true -DBUILD_PROJECTS=false"
#make clean
#make
