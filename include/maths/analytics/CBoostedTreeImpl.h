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

#ifndef INCLUDED_ml_maths_analytics_CBoostedTreeImpl_h
#define INCLUDED_ml_maths_analytics_CBoostedTreeImpl_h

#include <core/CDataFrame.h>
#include <core/CMemory.h>
#include <core/CPackedBitVector.h>
#include <core/CStatePersistInserter.h>
#include <core/CStateRestoreTraverser.h>

#include <maths/analytics/CBoostedTree.h>
#include <maths/analytics/CBoostedTreeHyperparameters.h>
#include <maths/analytics/CBoostedTreeLeafNodeStatistics.h>
#include <maths/analytics/CBoostedTreeLoss.h>
#include <maths/analytics/CBoostedTreeUtils.h>
#include <maths/analytics/CDataFrameAnalysisInstrumentationInterface.h>
#include <maths/analytics/CDataFrameCategoryEncoder.h>
#include <maths/analytics/CDataFrameUtils.h>
#include <maths/analytics/ImportExport.h>

#include <maths/common/CBasicStatistics.h>
#include <maths/common/CLinearAlgebraEigen.h>
#include <maths/common/CPRNG.h>

#include <boost/optional.hpp>

#include <limits>
#include <memory>
#include <numeric>
#include <sstream>
#include <utility>
#include <vector>

namespace ml {
namespace maths {
namespace common {
class CBayesianOptimisation;
}
namespace analytics {
class CBoostedTreeImplForTest;
class CTreeShapFeatureImportance;

//! \brief Implementation of CBoostedTree.
class MATHS_ANALYTICS_EXPORT CBoostedTreeImpl final {
public:
    using TDoubleVec = std::vector<double>;
    using TSizeVec = std::vector<std::size_t>;
    using TStrVec = std::vector<std::string>;
    using TOptionalDouble = boost::optional<double>;
    using TStrDoublePrVec = std::vector<std::pair<std::string, double>>;
    using TOptionalStrDoublePrVec = boost::optional<TStrDoublePrVec>;
    using TVector = common::CDenseVector<double>;
    using TMeanAccumulator = common::CBasicStatistics::SSampleMean<double>::TAccumulator;
    using TMeanVarAccumulator = common::CBasicStatistics::SSampleMeanVar<double>::TAccumulator;
    using TMeanVarAccumulatorSizeDoubleTuple =
        std::tuple<TMeanVarAccumulator, std::size_t, double>;
    using TMeanVarAccumulatorVec = std::vector<TMeanVarAccumulator>;
    using TBayesinOptimizationUPtr = std::unique_ptr<common::CBayesianOptimisation>;
    using TNodeVec = CBoostedTree::TNodeVec;
    using TNodeVecVec = CBoostedTree::TNodeVecVec;
    using TLossFunction = boosted_tree::CLoss;
    using TLossFunctionUPtr = CBoostedTree::TLossFunctionUPtr;
    using TTrainingStateCallback = CBoostedTree::TTrainingStateCallback;
    using TRegularization = CBoostedTreeRegularization<double>;
    using TAnalysisInstrumentationPtr = CDataFrameTrainBoostedTreeInstrumentationInterface*;
    using THyperparameterImportanceVec =
        std::vector<boosted_tree_detail::SHyperparameterImportance>;

public:
    static const double MINIMUM_RELATIVE_GAIN_PER_SPLIT;

public:
    CBoostedTreeImpl(std::size_t numberThreads,
                     TLossFunctionUPtr loss,
                     TAnalysisInstrumentationPtr instrumentation = nullptr);
    CBoostedTreeImpl(CBoostedTreeImpl&&);

    ~CBoostedTreeImpl();

    CBoostedTreeImpl& operator=(const CBoostedTreeImpl&) = delete;
    CBoostedTreeImpl& operator=(CBoostedTreeImpl&&);

    //! Train the model on the values in \p frame.
    void train(core::CDataFrame& frame, const TTrainingStateCallback& recordTrainStateCallback);

    //! Write the predictions of the best trained model to \p frame.
    //!
    //! \warning Must be called only if a trained model is available.
    void predict(core::CDataFrame& frame) const;

    //! Get the SHAP value calculator.
    //!
    //! \warning Will return a nullptr if a trained model isn't available.
    CTreeShapFeatureImportance* shap();

    //! Get the vector of hyperparameter importances.
    THyperparameterImportanceVec hyperparameterImportance() const;

    //! Get the model produced by training if it has been run.
    const TNodeVecVec& trainedModel() const;

    //! Get the training loss function.
    TLossFunction& loss() const;

    //! Get the column containing the dependent variable.
    std::size_t columnHoldingDependentVariable() const;

    //! Get start indices of the extra columns.
    const TSizeVec& extraColumns() const;

    //! Get the weights to apply to each class's predicted probability when
    //! assigning classes.
    const TVector& classificationWeights() const;

    //! Get the memory used by this object.
    std::size_t memoryUsage() const;

    //! Estimate the maximum booking memory that training the boosted tree on a data
    //! frame with \p numberRows row and \p numberColumns columns will use.
    std::size_t estimateMemoryUsage(std::size_t numberRows, std::size_t numberColumns) const;

    //! Correct from worst case memory usage to a more realistic estimate.
    static std::size_t correctedMemoryUsage(double memoryUsageBytes);

    //! Persist by passing information to \p inserter.
    void acceptPersistInserter(core::CStatePersistInserter& inserter) const;

    //! Populate the object from serialized data.
    bool acceptRestoreTraverser(core::CStateRestoreTraverser& traverser);

    //! Visit this tree trainer implementation.
    void accept(CBoostedTree::CVisitor& visitor);

    //! \return The best hyperparameters for validation error found so far.
    const CBoostedTreeHyperparameters& bestHyperparameters() const;

    //! \return The fraction of data we use for train per fold when tuning hyperparameters.
    double trainFractionPerFold() const;

    //! \return The full training set data mask, i.e. all rows which aren't missing
    //! the dependent variable.
    core::CPackedBitVector allTrainingRowsMask() const;

    //!\ name Test Only
    //@{
    //! The name of the object holding the best hyperaparameters in the state document.
    static const std::string& bestHyperparametersName();

    //! The name of the object holding the best regularisation hyperparameters in the
    //! state document.
    static const std::string& bestRegularizationHyperparametersName();

    //! A list of the names of the best individual hyperparameters in the state document.
    static TStrVec bestHyperparameterNames();

    //! Get the threshold on the predicted probability of class one at which to
    //!
    //! Get the feature sample probabilities.
    const TDoubleVec& featureSampleProbabilities() const;
    //@}

private:
    using TDoubleDoublePr = std::pair<double, double>;
    using TOptionalDoubleVec = std::vector<TOptionalDouble>;
    using TOptionalDoubleVecVec = std::vector<TOptionalDoubleVec>;
    using TOptionalSize = boost::optional<std::size_t>;
    using TFloatVec = std::vector<common::CFloatStorage>;
    using TFloatVecVec = std::vector<TFloatVec>;
    using TPackedBitVectorVec = std::vector<core::CPackedBitVector>;
    using TNodeVecVecDoubleDoubleVecTuple = std::tuple<TNodeVecVec, double, TDoubleVec>;
    using TDataFrameCategoryEncoderUPtr = std::unique_ptr<CDataFrameCategoryEncoder>;
    using TDataTypeVec = CDataFrameUtils::TDataTypeVec;
    using TRegularizationOverride = CBoostedTreeRegularization<TOptionalDouble>;
    using TTreeShapFeatureImportanceUPtr = std::unique_ptr<CTreeShapFeatureImportance>;
    using TWorkspace = CBoostedTreeLeafNodeStatistics::CWorkspace;
    using THyperparametersVec = std::vector<boosted_tree_detail::EHyperparameters>;
    using TDoubleVecVec = std::vector<TDoubleVec>;

    //! Tag progress through initialization.
    enum EInitializationStage {
        E_NotInitialized = 0,
        E_SoftTreeDepthLimitInitialized = 1,
        E_DepthPenaltyMultiplierInitialized = 2,
        E_TreeSizePenaltyMultiplierInitialized = 3,
        E_LeafWeightPenaltyMultiplierInitialized = 4,
        E_DownsampleFactorInitialized = 5,
        E_FeatureBagFractionInitialized = 6,
        E_EtaInitialized = 7,
        E_FullyInitialized = 8
    };

private:
    CBoostedTreeImpl();

    //! Check if we can train a model.
    bool canTrain() const;

    //! Get the mean number of training examples which are used in each fold.
    double meanNumberTrainingRowsPerFold() const;

    //! Compute the \p percentile percentile gain per split and the sum of row
    //! curvatures per internal node of \p forest.
    TDoubleDoublePr gainAndCurvatureAtPercentile(double percentile,
                                                 const TNodeVecVec& forest) const;

    //! Presize the collection to hold the per fold test errors.
    void initializePerFoldTestLosses();

    //! Compute the probability threshold at which to classify a row as class one.
    void computeClassificationWeights(const core::CDataFrame& frame);

    //! Prepare to calculate SHAP feature importances.
    void initializeTreeShap(const core::CDataFrame& frame);

    //! Train the forest and compute loss moments on each fold.
    TMeanVarAccumulatorSizeDoubleTuple crossValidateForest(core::CDataFrame& frame);

    //! Initialize the predictions and loss function derivatives for the masked
    //! rows in \p frame.
    TNodeVec initializePredictionsAndLossDerivatives(core::CDataFrame& frame,
                                                     const core::CPackedBitVector& trainingRowMask,
                                                     const core::CPackedBitVector& testingRowMask) const;

    //! Train one forest on the rows of \p frame in the mask \p trainingRowMask.
    TNodeVecVecDoubleDoubleVecTuple
    trainForest(core::CDataFrame& frame,
                const core::CPackedBitVector& trainingRowMask,
                const core::CPackedBitVector& testingRowMask,
                core::CLoopProgress& trainingProgress) const;

    //! Randomly downsamples the training row mask by the downsample factor.
    core::CPackedBitVector downsample(const core::CPackedBitVector& trainingRowMask) const;

    //! Get the candidate splits values for each feature.
    TFloatVecVec candidateSplits(const core::CDataFrame& frame,
                                 const core::CPackedBitVector& trainingRowMask) const;

    //! Updates the row's cached splits if the candidate splits have changed.
    void refreshSplitsCache(core::CDataFrame& frame,
                            const TFloatVecVec& candidateSplits,
                            const core::CPackedBitVector& trainingRowMask) const;

    //! Train one tree on the rows of \p frame in the mask \p trainingRowMask.
    TNodeVec trainTree(core::CDataFrame& frame,
                       const core::CPackedBitVector& trainingRowMask,
                       const TFloatVecVec& candidateSplits,
                       const std::size_t maximumTreeSize,
                       TWorkspace& workspace) const;

    //! Compute the minimum mean test loss per fold for any round.
    double minimumTestLoss() const;

    //! Estimate the loss we'll get including the missing folds.
    TMeanVarAccumulator correctTestLossMoments(const TSizeVec& missing,
                                               TMeanVarAccumulator lossMoments) const;

    //! Estimate test losses for the \p missing folds.
    TMeanVarAccumulatorVec estimateMissingTestLosses(const TSizeVec& missing) const;

    //! Get the minimum number of rows we require per feature.
    std::size_t rowsPerFeature(std::size_t numberRows) const;

    //! Get the number of features including category encoding.
    std::size_t numberFeatures() const;

    //! Get the number of features to consider splitting on.
    std::size_t featureBagSize(double fractionMultiplier) const;

    //! Sample the features according to their categorical distribution.
    void treeFeatureBag(TDoubleVec& probabilities, TSizeVec& treeFeatureBag) const;

    //! Sample the features according to their categorical distribution.
    void nodeFeatureBag(const TSizeVec& treeFeatureBag,
                        TDoubleVec& probabilities,
                        TSizeVec& nodeFeatureBag) const;

    //! Get a column mask of the suitable regressor features.
    void candidateRegressorFeatures(const TDoubleVec& probabilities, TSizeVec& features) const;

    //! Refresh the predictions and loss function derivatives for the masked
    //! rows in \p frame with predictions of \p tree.
    void refreshPredictionsAndLossDerivatives(core::CDataFrame& frame,
                                              const core::CPackedBitVector& trainingRowMask,
                                              const core::CPackedBitVector& testingRowMask,
                                              double eta,
                                              double lambda,
                                              TNodeVec& tree) const;

    //! Compute the mean of the loss function on the masked rows of \p frame.
    double meanLoss(const core::CDataFrame& frame, const core::CPackedBitVector& rowMask) const;

    //! Compute the overall variance of the error we see between folds.
    double betweenFoldTestLossVariance() const;

    //! Get the root node of \p tree.
    static const CBoostedTreeNode& root(const TNodeVec& tree);

    //! Get the root node of \p tree.
    static CBoostedTreeNode& root(TNodeVec& tree);

    //! Get the forest's prediction for \p row.
    TVector predictRow(const CEncodedDataFrameRowRef& row, const TNodeVecVec& forest) const;

    //! Select the next hyperparameters for which to train a model.
    bool selectNextHyperparameters(const TMeanVarAccumulator& lossMoments,
                                   common::CBayesianOptimisation& bopt);

    //! Capture the current hyperparameter values.
    void captureBestHyperparameters(const TMeanVarAccumulator& lossMoments,
                                    std::size_t maximumNumberTrees,
                                    double numberNodes);

    //! Set the hyperparamaters from the best recorded.
    void restoreBestHyperparameters();

    //! Scale the regulariser multipliers by \p scale.
    void scaleRegularizers(double scale);

    //! Check invariants which are assumed to hold after restoring.
    void checkRestoredInvariants() const;

    //! Check invariants which are assumed to hold in order to train on \p frame.
    void checkTrainInvariants(const core::CDataFrame& frame) const;

    //! Get the number of hyperparameters to tune.
    std::size_t numberHyperparametersToTune() const;

    //! Get the maximum number of nodes to use in a tree.
    //!
    //! \note This number will only be used if the regularised loss says its
    //! a good idea.
    std::size_t maximumTreeSize(const core::CPackedBitVector& trainingRowMask) const;

    //! Get the maximum number of nodes to use in a tree.
    //!
    //! \note This number will only be used if the regularised loss says its
    //! a good idea.
    std::size_t maximumTreeSize(std::size_t numberRows) const;

    //! Get the maximum memory of any trained model we will produce.
    //!
    //! This is the largest model we will train if there is a limit on the
    //! deployed model.
    std::size_t maximumTrainedModelSize() const;

    //! Start monitoring fine tuning hyperparameters.
    void startProgressMonitoringFineTuneHyperparameters();

    //! Start monitoring the final model training.
    void startProgressMonitoringFinalTrain();

    //! Skip monitoring the final model training.
    void skipProgressMonitoringFinalTrain();

    //! Record the training state using the \p recordTrainState callback function
    void recordState(const TTrainingStateCallback& recordTrainState) const;

    //! Record hyperparameters for instrumentation.
    void recordHyperparameters();

    //! Populate the list of tunable hyperparameters.
    void initializeTunableHyperparameters();

    //! Use Sobol sampler for for random hyperparamers.
    void initializeHyperparameterSamples();

private:
    mutable common::CPRNG::CXorOShiro128Plus m_Rng;
    EInitializationStage m_InitializationStage = E_NotInitialized;
    std::size_t m_NumberThreads;
    std::size_t m_DependentVariable = std::numeric_limits<std::size_t>::max();
    std::size_t m_PaddedExtraColumns = 0;
    TSizeVec m_ExtraColumns;
    TLossFunctionUPtr m_Loss;
    CBoostedTree::EClassAssignmentObjective m_ClassAssignmentObjective =
        CBoostedTree::E_MinimumRecall;
    bool m_StopCrossValidationEarly = true;
    TRegularizationOverride m_RegularizationOverride;
    TOptionalDouble m_DownsampleFactorOverride;
    TOptionalDouble m_EtaOverride;
    TOptionalDouble m_EtaGrowthRatePerTreeOverride;
    TOptionalSize m_NumberFoldsOverride;
    TOptionalDouble m_TrainFractionPerFoldOverride;
    TOptionalSize m_MaximumNumberTreesOverride;
    TOptionalDouble m_FeatureBagFractionOverride;
    TOptionalStrDoublePrVec m_ClassificationWeightsOverride;
    TRegularization m_Regularization;
    TVector m_ClassificationWeights;
    double m_DownsampleFactor = 0.5;
    double m_Eta = 0.1;
    double m_EtaGrowthRatePerTree = 1.05;
    std::size_t m_NumberFolds = 4;
    double m_TrainFractionPerFold = 0.75;
    std::size_t m_MaximumNumberTrees = 20;
    std::size_t m_MaximumAttemptsToAddTree = 3;
    std::size_t m_MaximumDeployedSize = std::numeric_limits<std::size_t>::max();
    std::size_t m_NumberSplitsPerFeature = 75;
    std::size_t m_MaximumOptimisationRoundsPerHyperparameter = 2;
    std::size_t m_RowsPerFeature = 50;
    double m_FeatureBagFraction = 0.5;
    TDataFrameCategoryEncoderUPtr m_Encoder;
    TDataTypeVec m_FeatureDataTypes;
    TDoubleVec m_FeatureSampleProbabilities;
    TPackedBitVectorVec m_MissingFeatureRowMasks;
    TPackedBitVectorVec m_TrainingRowMasks;
    TPackedBitVectorVec m_TestingRowMasks;
    double m_BestForestTestLoss = boosted_tree_detail::INF;
    TOptionalDoubleVecVec m_FoldRoundTestLosses;
    CBoostedTreeHyperparameters m_BestHyperparameters;
    TNodeVecVec m_BestForest;
    TBayesinOptimizationUPtr m_BayesianOptimization;
    std::size_t m_NumberRounds = 1;
    std::size_t m_CurrentRound = 0;
    core::CLoopProgress m_TrainingProgress;
    std::size_t m_NumberTopShapValues = 0;
    TTreeShapFeatureImportanceUPtr m_TreeShap;
    TAnalysisInstrumentationPtr m_Instrumentation;
    TMeanAccumulator m_MeanForestSizeAccumulator;
    TMeanAccumulator m_MeanLossAccumulator;
    THyperparametersVec m_TunableHyperparameters;
    TDoubleVecVec m_HyperparameterSamples;
    bool m_StopHyperparameterOptimizationEarly = true;

private:
    friend class CBoostedTreeFactory;
    friend class CBoostedTreeImplForTest;
};
}
}
}

#endif // INCLUDED_ml_maths_analytics_CBoostedTreeImpl_h
