#include "uuid.h"

#include <random>

// TODO: pull out to some global random header?
static std::random_device rd{};
static std::seed_seq ss{rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
static std::mt19937_64 mt{ss};

UUID::UUID() : m_value{mt()} {}
