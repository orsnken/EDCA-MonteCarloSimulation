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
  enum Type {TYPE_AP, TYPE_STA,};
  Type type;
  int  aifs;
  int  cw_min;
  int  cw_max;
  EdcaParam(Type a_type, int a_aifs = 0, int a_cw_min = 0, int a_cw_max = 0) :
    type(a_type), aifs(a_aifs), cw_min(a_cw_min), cw_max(a_cw_max) {}
};
EdcaParam argEdcaParamAp, argEdcaParamSta;

class Edca {
public:
  Edca(EdcaParam ep, int ssid, int mac) :
  ep_(ep), ssid_(ssid), mac_(mac) {
    calcBackoff();
  }

  int decrement_backoff() {
    assert(backoff_ > 0);
    return --backoff_;
  }

  int transmitting_successed() {
    ntsucc_++;
    nfail_ = 0;
    return calcBackoff();
  }

  int transmitting_failed() {
    ntfail_++;
    nfail_++;
    return calcBackoff();
  }

  int backoff() const {
    return backoff_;
  }

  int ssid() const {
    return ssid_;
  }

  int mac() const {
    return mac_;
  }

  const EdcaParam& param() const {
    return ep_;
  }

private:
  const int ssid_;
  const int mac_;
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
      argEdcaParamAp  = EdcaParam( EdcaParam::TYPE_AP,  ap_aifs,  ap_cw_min,  ap_cw_max);
      argEdcaParamSta = EdcaParam(EdcaParam::TYPE_STA, sta_aifs, sta_cw_min, sta_cw_max);
      init();
      run();
    } catch (const std::invalid_argument& e) {
      std::cerr << "[arguments error]arguments must be positive integer." << std::endl;
    }
  }
}

std::vector<Edca *> aps;
std::vector<Edca *> stas;

void init() {
  aps.push_back(new Edca(argEdcaParamAp, 0, 10));
  aps.push_back(new Edca(argEdcaParamAp, 1, 20));
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < argN; j++) {
      stas.push_back(new Edca(argEdcaParamSta, i, 10 * (i + 1) + j));
    }
  }
}


void run() {
  for (int i = 0; i < argRep; i++) {
    simulate();
  }
}

void simulate() {
  std::vector<Edca *> trns_queue;
  int daifs = argEdcaParamAp.aifs - argEdcaParamSta.aifs;
  
  // --------------------------------
  // STAs Contention Period
  // --------------------------------
  for (int i = 0; i < daifs && trns_queue.size() == 0; i++) {
    for (Edca *sta: stas) {
      if (sta->backoff() == 0) {
        trns_queue.push_back(sta);
      }
    }
    if (trns_queue.size() > 0) {
      for (Edca *sta: stas) {
        sta->decrement_backoff();
      }
    }
  }

  // --------------------------------
  // APs and STAs Contention Period
  // --------------------------------
  std::vector<Edca *> terminals = aps;
  terminals.insert(terminals.end(), stas.begin(), stas.end());
  while (trns_queue.size() == 0) {
    for (Edca *t: terminals) {
      if (t->backoff() == 0) {
        trns_queue.push_back(t);
      }
    }
    if (trns_queue.size() > 0) {
      for (Edca *t: terminals) {
        t->decrement_backoff();
      }
    }
  }

  // --------------------------------
  // Transmitting are Successed or Failed
  // --------------------------------
  for (Edca *qe: trns_queue) {
    bool successed = true;
    for (Edca *t: trns_queue) {
      if (qe->mac() != t->mac()) {
        if (qe->ssid() == t->ssid()) {
          successed = false;
          break;
        }
      }
      if (successed) {
        qe->transmitting_successed();
      } else {
        qe->transmitting_failed();
      }
    }
  }
}
