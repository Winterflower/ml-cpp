#
# Copyright Elasticsearch B.V. and/or licensed to Elasticsearch B.V. under one
# or more contributor license agreements. Licensed under the Elastic License
# 2.0 and the following additional limitation. Functionality enabled by the
# files subject to the Elastic License 2.0 may only be used in production when
# invoked by an Elasticsearch process with a license key installed that permits
# use of machine learning features. You may not use this file except in
# compliance with the Elastic License 2.0 and the foregoing additional
# limitation.
#
include $(CPP_SRC_HOME)/mk/defines.mk

TARGET=$(OBJS_DIR)/libMlMathsTimeSeries$(DYNAMIC_LIB_EXT)

USE_BOOST=1
USE_RAPIDJSON=1
USE_EIGEN=1

LIBS:=$(LIB_ML_CORE) $(LIB_ML_MATHS_COMMON)

all: build

SRCS= \
CAdaptiveBucketing.cc \
CCalendarComponent.cc \
CCalendarComponentAdaptiveBucketing.cc \
CCalendarCyclicTest.cc \
CCalendarFeature.cc \
CCountMinSketch.cc \
CDecayRateController.cc \
CDecompositionComponent.cc \
CExpandingWindow.cc \
CModelStateSerialiser.cc \
CSeasonalComponent.cc \
CSeasonalComponentAdaptiveBucketing.cc \
CSeasonalTime.cc \
CSignal.cc \
CTimeSeriesDecomposition.cc \
CTimeSeriesDecompositionDetail.cc \
CTimeSeriesDecompositionStateSerialiser.cc \
CTimeSeriesDecompositionStub.cc \
CTimeSeriesModel.cc \
CTimeSeriesMultibucketFeatureSerialiser.cc \
CTimeSeriesSegmentation.cc \
CTimeSeriesTestForChange.cc \
CTimeSeriesTestForSeasonality.cc \
CTrendComponent.cc \

include $(CPP_SRC_HOME)/mk/dynamiclib.mk

