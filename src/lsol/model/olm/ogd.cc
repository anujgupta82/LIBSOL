/*********************************************************************************
*     File Name           :     ogd.cc
*     Created By          :     yuewu
*     Creation Date       :     [2016-02-18 21:37]
*     Last Modified       :     [2016-03-09 19:22]
*     Description         :     Stochastic Gradient Descent
**********************************************************************************/

#include "lsol/model/olm/ogd.h"
#include <cmath>

using namespace std;
using namespace lsol::math::expr;

namespace lsol {

namespace model {
OGD::OGD(int class_num) : OnlineLinearModel(class_num), eta0_(1) {
  this->set_power_t(0.5);
}
void OGD::SetParameter(const std::string& name, const std::string& value) {
  if (name == "power_t") {
    this->set_power_t(stof(value));
  } else if (name == "eta") {
    this->eta0_ = stof(value);
    Check(eta0_ >= 0);
  } else {
    OnlineLinearModel::SetParameter(name, value);
  }
}
void OGD::Update(const pario::DataPoint& dp, const float*, float) {
  const auto& x = dp.data();
  eta_ = eta0_ / this->pow_(this->cur_iter_num_, this->power_t_);

  for (int c = 0; c < this->clf_num_; ++c) {
    if (g(c) == 0) continue;
    w(c) -= this->eta_ * g(c) * x;
    // update bias
    w(c)[0] -= bias_eta() * g(c);
  }
}

void OGD::GetModelInfo(Json::Value& root) const {
  OnlineLinearModel::GetModelInfo(root);
  root["online"]["power_t"] = this->power_t_;
  root["online"]["eta"] = this->eta0_;
}

// calculate power t
float pow_const(int iter, float power_t) { return 1; }
float pow_sqrt(int iter, float power_t) { return sqrtf(float(iter)); }
float pow_linear(int iter, float power_t) { return float(iter); }
float pow_general(int iter, float power_t) {
  return powf((float)iter, power_t);
}

void OGD::set_power_t(float power_t) {
  Check(power_t >= 0);
  this->power_t_ = power_t;
  if (power_t == 0)
    this->pow_ = pow_const;
  else if (power_t == 0.5)
    this->pow_ = pow_sqrt;
  else if (power_t == 1)
    this->pow_ = pow_linear;
  else
    this->pow_ = pow_general;
}

RegisterModel(OGD, "ogd", "Online Gradient Descent");

STG::STG(int class_num) : OGD(class_num), k_(1) { this->regularizer_ = &l1_; }

void STG::SetParameter(const std::string& name, const std::string& value) {
  if (name == "k") {
    this->k_ = stoi(value);
    Check(k_ > 0);
  } else {
    OGD::SetParameter(name, value);
  }
}

void STG::BeginTrain() {
  OGD::BeginTrain();
  this->last_trunc_time_.resize(this->dim_);
  this->last_trunc_time_ = real_t(this->initial_t_);
}

label_t STG::TrainPredict(const pario::DataPoint& dp, float* predicts) {
  const auto& x = dp.data();
  size_t feat_num = x.size();
  int t = cur_iter_num_ - 1;
  float alpha = this->eta_ * l1_.lambda();
  for (int c = 0; c < this->clf_num_; ++c) {
    auto& w1 = w(c);
    // trucate weights
    for (size_t i = 0; i < feat_num; ++i) {
      index_t idx = x.index(i);
      int step = int(t - last_trunc_time_[idx]);
      if (step < k_) continue;
      step -= step % k_;
      w1[idx] = truncate(w1[idx], step * alpha);
      last_trunc_time_[idx] += step;
    }
    // truncate bias
    w1[0] = truncate(w1[0], l1_.lambda() * bias_eta());
    last_trunc_time_[0] = real_t(t);
  }
  return OnlineLinearModel::TrainPredict(dp, predicts);
}

void STG::EndTrain() {
  real_t t = real_t(cur_iter_num_);
  for (int c = 0; c < this->clf_num_; ++c) {
    // trucate weights
    w(c) = truncate(w(c), (eta_ * l1_.lambda()) * (t - last_trunc_time_));
  }
  OGD::EndTrain();
}

void STG::update_dim(index_t dim) {
  if (dim > this->dim_) {
    this->last_trunc_time_.resize(dim);
    real_t t0 = real_t(this->initial_t_);
    this->last_trunc_time_.slice_op([t0](real_t& val) { val = t0; },
                                    this->dim_);
    OGD::update_dim(dim);
  }
}

void STG::GetModelInfo(Json::Value& root) const {
  OGD::GetModelInfo(root);
  root["online"]["k"] = this->k_;
}

RegisterModel(STG, "stg", "Sparse Online Learning via Truncated Gradient");

FOBOS_L1::FOBOS_L1(int class_num) : OGD(class_num) {
  this->regularizer_ = &l1_;
}

label_t FOBOS_L1::TrainPredict(const pario::DataPoint& dp, float* predicts) {
  const auto& x = dp.data();
  real_t t = real_t(cur_iter_num_ - 1);
  for (int c = 0; c < this->clf_num_; ++c) {
    // trucate weights
    w(c) = truncate(w(c).slice(x),
                    (eta_ * l1_.lambda()) * (t - l1_.last_update_time()));
    // truncate bias
    w(c)[0] = truncate(w(c)[0], bias_eta() * l1_.lambda());
  }

  return OnlineLinearModel::TrainPredict(dp, predicts);
}

void FOBOS_L1::EndTrain() {
  real_t t = real_t(cur_iter_num_);
  for (int c = 0; c < this->clf_num_; ++c) {
    // trucate weights
    w(c) = truncate(w(c), (eta_ * l1_.lambda()) * (t - l1_.last_update_time()));
    // truncate bias
    w(c)[0] = truncate(w(c)[0], bias_eta() * l1_.lambda());
  }
  OGD::EndTrain();
}

RegisterModel(FOBOS_L1, "fobos-l1",
              "Forward Backward Splitting l1 regularization");

}  // namespace model
}  // namespace lsol
