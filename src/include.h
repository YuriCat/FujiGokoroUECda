#pragma once

// 共通インクルード

#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include <ctime>

#include <cmath>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cassert>
#include <thread>
#include <mutex>
#include <atomic>
#include <array>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <random>
#include <algorithm>
#include <string>
#include <bitset>
#include <unordered_map>
#include <unordered_set>

#ifdef _WIN32
/*ここからwindows版インクルード*/
#include <winsock2.h>
#include <ws2tcpip.h>
/*ここまで*/
#else
/*ここからunix版インクルード*/
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
/*ここまで*/
#endif

// 設定
#include "settings.h"

#if defined(MINIMUM)
#define EXTENDED_VARIABLE(x) ((void)x)
#else
#define EXTENDED_VARIABLE(x) (x)
#endif

// DERR...通常は消す
// CERR...本番用以外では消さない

// 基本的定義、ユーティリティ
#include "../CppCommon/src/util/string.hpp"
#include "../CppCommon/src/util/random.hpp"
#include "../CppCommon/src/util/bitOperation.hpp"
#include "../CppCommon/src/util/container.hpp"
#include "../CppCommon/src/util/arrays.h"
#include "../CppCommon/src/util/bitSet.hpp"
#include "../CppCommon/src/util/bitArray.hpp"
#include "../CppCommon/src/util/atomic.hpp"
#include "../CppCommon/src/util/bitPartition.hpp"
#include "../CppCommon/src/util/math.hpp"
#include "../CppCommon/src/hash/hashFunc.hpp"
#include "../CppCommon/src/hash/hashBook.hpp"
#include "../CppCommon/src/util/softmaxClassifier.hpp"
#include "../CppCommon/src/util/selection.hpp"
#include "../CppCommon/src/util/lock.hpp"
#include "../CppCommon/src/util/io.hpp"
#include "../CppCommon/src/util/pd.hpp"
#include "../CppCommon/src/util/statistics.hpp"
#include "../CppCommon/src/util/index.hpp"
#include "../CppCommon/src/analyze/analyzer.hpp"

// ゲームデータ
#include "UECda.h"

// 基本型
#include "core/daifugo.hpp"
#include "core/prim2.hpp"
#include "core/hand.hpp"
#include "core/field.hpp"
#include "core/record.hpp"
#include "core/hash.hpp"

using namespace UECda;