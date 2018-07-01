#include <iostream>
#include <random>
#include <string>
#include <stdexcept>
#include <vector>

#include <cassert>
#include <cmath>

std::random_device rnd;
std::mt19937 rnd_gen(rnd());

int argN;
int argRep;

struct EdcaParam {
  int aifs;
  int cw_min;
  int cw_max;
  EdcaParam(int a_aifs = 0, int a_cw_min = 0, int a_cw_max = 0) :
    aifs(a_aifs), cw_min(a_cw_min), cw_max(a_cw_max) {}
};
EdcaParam argEdcaParamAp, argEdcaParamSta;

class Edca {
public:
  Edca(EdcaParam ep) : ep_(ep) {
    calcBackoff();
  }

  int decrement_backoff() {
    assert(backoff_ > 0);
    return --backoff_;
  }

  int transmition_successed() {
    ntsucc_++;
    nfail_ = 0;
    return calcBackoff();
  }

  int transmition_failed() {
    ntfail_++;
    nfail_++;
    return calcBackoff();
  }

private:
  const EdcaParam ep_;
  int backoff_;
  int nfail_;
  int ntsucc_;
  int ntfail_;

  int calcBackoff() {
    int cw = (ep_.cw_min + 1) * std::pow(2, nfail_) - 1;
    cw = std::min(cw, ep_.cw_max);
    std::uniform_int_distribution<int> cw_gen(0, cw);
    backoff_ = cw_gen(rnd_gen);
    assert(backoff_ <= ep_.cw_max);
    return backoff_;
  }
};

void init();
void run();
void simulate();

int main(int argc, char **argv) {
  if (argc - 1 != 8) {
    std::cerr << "[arguments error]" << std::endl;
    std::cerr << "./a [num. of edca-wlan-sta][Num. of iter.][ap_aifs][ap_cw_min][ap_cw_max][sta_aifs][sta_cw_min][sta_cw_max]" << std::endl;
  } else {
    try {
      int ap_aifs   = std::stoi(argv[3]);
      int ap_cw_min = std::stoi(argv[4]);
      int ap_cw_max = std::stoi(argv[5]);
      int sta_aifs   = std::stoi(argv[6]);
      int sta_cw_min = std::stoi(argv[7]);
      int sta_cw_max = std::stoi(argv[8]);
      argN = std::stoi(argv[1]);
      argRep = std::stoi(argv[2]);
      argEdcaParamAp  = EdcaParam( ap_aifs,  ap_cw_min,  ap_cw_max);
      argEdcaParamSta = EdcaParam(sta_aifs, sta_cw_min, sta_cw_max);
      init();
      run();
    } catch (const std::invalid_argument& e) {
      std::cerr << "[arguments error]arguments must be positive integer." << std::endl;
    }
  }
}

std::vector<Edca *> ap;
std::vector<Edca *> sta;

void run() {
  for (int i = 0; i < argRep; i++) {
    simulate();
  }
}

void simulate() {

}

void init() {
  ap.push_back(new Edca(argEdcaParamAp));
  ap.push_back(new Edca(argEdcaParamAp));
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < argN; j++) {
      sta.push_back(new Edca(argEdcaParamSta));
    }
  }
}
