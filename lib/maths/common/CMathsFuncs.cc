/*
 * Copyright Elasticsearch B.V. and/or licensed to Elasticsearch B.V. under one
 * or more contributor license agreements. Licensed under the Elastic License
 * 2.0 and the following additional limitation. Functionality enabled by the
 * files subject to the Elastic License 2.0 may only be used in production when
 * invoked by an Elasticsearch process with a license key installed that permits
 * use of machine learning features. You may not use this file except in
 * compliance with the Elastic License 2.0 and the foregoing additional
 * limitation.
 */

#include <maths/common/CMathsFuncs.h>

#include <maths/common/CMathsFuncsForMatrixAndVectorTypes.h>

#include <cmath>

namespace ml {
namespace maths {
namespace common {
bool CMathsFuncs::isNan(double val) {
    return std::isnan(val);
}
bool CMathsFuncs::isNan(const CSymmetricMatrix<double>& val) {
    return anElement(static_cast<bool (*)(double)>(&isNan), val);
}
bool CMathsFuncs::isNan(const CVector<double>& val) {
    return aComponent(static_cast<bool (*)(double)>(&isNan), val);
}
bool CMathsFuncs::isNan(const core::CSmallVectorBase<double>& val) {
    for (std::size_t i = 0; i < val.size(); ++i) {
        if (isNan(val[i])) {
            return true;
        }
    }
    return false;
}

bool CMathsFuncs::isInf(double val) {
    return std::isinf(val);
}
bool CMathsFuncs::isInf(const CVector<double>& val) {
    return aComponent(static_cast<bool (*)(double)>(&isInf), val);
}
bool CMathsFuncs::isInf(const CSymmetricMatrix<double>& val) {
    return anElement(static_cast<bool (*)(double)>(&isInf), val);
}
bool CMathsFuncs::isInf(const core::CSmallVectorBase<double>& val) {
    for (std::size_t i = 0; i < val.size(); ++i) {
        if (isInf(val[i])) {
            return true;
        }
    }
    return false;
}

bool CMathsFuncs::isFinite(double val) {
    return std::isfinite(val);
}
bool CMathsFuncs::isFinite(const CVector<double>& val) {
    return everyComponent(static_cast<bool (*)(double)>(&isFinite), val);
}
bool CMathsFuncs::isFinite(const CSymmetricMatrix<double>& val) {
    return everyElement(static_cast<bool (*)(double)>(&isFinite), val);
}
bool CMathsFuncs::isFinite(const core::CSmallVectorBase<double>& val) {
    for (std::size_t i = 0; i < val.size(); ++i) {
        if (!isFinite(val[i])) {
            return false;
        }
    }
    return true;
}

maths_t::EFloatingPointErrorStatus CMathsFuncs::fpStatus(double val) {
    if (isNan(val)) {
        return maths_t::E_FpFailed;
    }
    if (isInf(val)) {
        return maths_t::E_FpOverflowed;
    }
    return maths_t::E_FpNoErrors;
}
}
}
}
