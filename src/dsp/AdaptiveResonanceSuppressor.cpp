#include "AdaptiveResonanceSuppressor.h"

#include <algorithm>
#include <cmath>

namespace a125::drum::dsp {

namespace {

constexpr double kEpsilon = 1.0e-18;
constexpr double kDetectorQ = 5.0;
constexpr double kMaximumReduction = 0.24; // provisional; drum fixture QA required
constexpr double kMinimumDominance = 0.065;
constexpr double kPeakinessStart = 1.70;
constexpr double kPeakinessFull = 3.80;

double safeSquare(double value) noexcept {
    const double bounded =
        std::min(
            std::abs(value),
            1.0e100);

    return bounded * bounded;
}

}

double AdaptiveResonanceSuppressor::timeCoefficient(
    double sampleRate,
    double milliseconds) noexcept {

    const double fs =
        std::max(
            sampleRate,
            1000.0);

    const double ms =
        std::max(
            milliseconds,
            0.01);

    return std::exp(
        -1.0 /
        (fs * ms * 0.001));
}

void AdaptiveResonanceSuppressor::prepare(
    double sampleRate) noexcept {

    sampleRate_ =
        std::isfinite(sampleRate) &&
        sampleRate > 1000.0
            ? sampleRate
            : 44100.0;

    for (std::size_t channel = 0;
         channel < detectors_.size();
         ++channel) {

        for (std::size_t band = 0;
             band < kBandCount;
             ++band) {

            detectors_[channel][band].
                setCoefficients(
                    makeBandPass(
                        sampleRate_,
                        kCenters[band],
                        kDetectorQ));
        }
    }

    energyAttack_ =
        timeCoefficient(
            sampleRate_,
            24.0);

    energyRelease_ =
        timeCoefficient(
            sampleRate_,
            420.0);

    wideAttack_ =
        timeCoefficient(
            sampleRate_,
            35.0);

    wideRelease_ =
        timeCoefficient(
            sampleRate_,
            520.0);

    reductionAttack_ =
        timeCoefficient(
            sampleRate_,
            14.0);

    reductionRelease_ =
        timeCoefficient(
            sampleRate_,
            260.0);

    transientFastA_=timeCoefficient(sampleRate_,2.0);
    transientSlowA_=timeCoefficient(sampleRate_,100.0);
    // Extend the broadband attack guard slightly to protect high-frequency\n    // stick noise; hold and release windows remain unchanged.\n    transientDelaySamples_=static_cast<std::size_t>(sampleRate_*0.028);
    onsetHoldSamples_=static_cast<std::size_t>(sampleRate_*0.36);
    refractorySamples_=static_cast<std::size_t>(sampleRate_*0.07);
    reset();
}

void AdaptiveResonanceSuppressor::reset() noexcept {
    for (auto& channel :
         detectors_) {

        for (auto& filter :
             channel) {
            filter.reset();
        }
    }

    slowEnergy_.fill(0.0);
    reduction_.fill(0.0);
    targetReduction_.fill(0.0);

    wideEnergy_ = 0.0;
    updateCounter_ = 0;
    transientFast_=transientSlow_=0.0;
    samplesSinceOnset_=onsetHoldSamples_+1;
    lastOnset_=refractorySamples_+1;
}

void AdaptiveResonanceSuppressor::updateTargets() noexcept {
    targetReduction_.fill(0.0);

    struct Candidate {
        std::size_t band {0};
        double score {0.0};
        double reduction {0.0};
    };

    std::array<Candidate, kBandCount> candidates {};

    for (std::size_t band = 0;
         band < kBandCount;
         ++band) {

        const double center =
            std::max(
                slowEnergy_[band],
                kEpsilon);

        const double leftNeighbour =
            band > 0
                ? slowEnergy_[band - 1]
                : slowEnergy_[
                    std::min<std::size_t>(
                        1,
                        kBandCount - 1)];

        const double rightNeighbour =
            band + 1 < kBandCount
                ? slowEnergy_[band + 1]
                : slowEnergy_[band - 1];

        const double neighbour =
            std::max(
                std::sqrt(
                    std::max(
                        leftNeighbour,
                        kEpsilon) *
                    std::max(
                        rightNeighbour,
                        kEpsilon)),
                kEpsilon);

        const double peakiness =
            center / neighbour;

        const double dominance =
            std::sqrt(
                center /
                (wideEnergy_ +
                 kEpsilon));

        const double peakAmount =
            std::clamp(
                (peakiness -
                 kPeakinessStart) /
                (kPeakinessFull -
                 kPeakinessStart),
                0.0,
                1.0);

        const double dominanceAmount =
            std::clamp(
                (dominance -
                 kMinimumDominance) /
                0.18,
                0.0,
                1.0);

        candidates[band] = {
            band,
            peakiness *
                dominance,
            kMaximumReduction *
                peakAmount *
                dominanceAmount
        };
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const Candidate& a,
           const Candidate& b) {
            return a.score > b.score;
        });

    std::size_t selected = 0;

    for (const auto& candidate :
         candidates) {

        if (candidate.reduction <= 0.0)
            continue;

        bool adjacent = false;

        for (std::size_t band = 0;
             band < kBandCount;
             ++band) {

            if (targetReduction_[band] <=
                0.0) {
                continue;
            }

            const auto distance =
                candidate.band > band
                    ? candidate.band - band
                    : band - candidate.band;

            if (distance <= 1u) {
                adjacent = true;
                break;
            }
        }

        if (adjacent)
            continue;

        targetReduction_[candidate.band] =
            candidate.reduction;

        if (++selected >= 2u)
            break;
    }
}

void AdaptiveResonanceSuppressor::processFrame(
    double& left,
    double& right, double amount) noexcept {

    if (!std::isfinite(left))
        left = 0.0;

    if (!std::isfinite(right))
        right = 0.0;

    std::array<double, kBandCount>
        bandLeft {};

    std::array<double, kBandCount>
        bandRight {};

    for (std::size_t band = 0;
         band < kBandCount;
         ++band) {

        bandLeft[band] =
            detectors_[0][band].
                process(left);

        bandRight[band] =
            detectors_[1][band].
                process(right);

        const double magnitude =
            std::max(
                std::abs(
                    bandLeft[band]),
                std::abs(
                    bandRight[band]));

        const double target =
            safeSquare(
                magnitude);

        const double coefficient =
            target > slowEnergy_[band]
                ? energyAttack_
                : energyRelease_;

        slowEnergy_[band] =
            coefficient *
                slowEnergy_[band] +
            (1.0 - coefficient) *
                target;
    }

    // An onset-gated, decaying ring is a candidate for suppression.
    // A stationary musical note receives no permanent tonal cut.
    const double peak=std::max(std::abs(left),std::abs(right));
    transientFast_=transientFastA_*transientFast_+
        (1.0-transientFastA_)*peak;
    transientSlow_=transientSlowA_*transientSlow_+
        (1.0-transientSlowA_)*peak;
    if(lastOnset_<refractorySamples_+1)++lastOnset_;
    if(samplesSinceOnset_<onsetHoldSamples_+1)++samplesSinceOnset_;
    if(transientFast_>0.025 &&
       transientFast_>2.3*(transientSlow_+0.001) &&
       lastOnset_>refractorySamples_){
        samplesSinceOnset_=0;
        lastOnset_=0;
    }
    const double wideTarget =
        safeSquare(
            std::max(
                std::abs(left),
                std::abs(right)));

    const double wideCoefficient =
        wideTarget > wideEnergy_
            ? wideAttack_
            : wideRelease_;

    wideEnergy_ =
        wideCoefficient *
            wideEnergy_ +
        (1.0 - wideCoefficient) *
            wideTarget;

    if (++updateCounter_ >= 128u) {
        updateCounter_ = 0u;
        updateTargets();
    }

    double correctionLeft = 0.0;
    double correctionRight = 0.0;

    for (std::size_t band = 0;
         band < kBandCount;
         ++band) {

        const double coefficient =
            targetReduction_[band] >
                    reduction_[band]
                ? reductionAttack_
                : reductionRelease_;

        reduction_[band] =
            coefficient *
                reduction_[band] +
            (1.0 - coefficient) *
                targetReduction_[band];

        correctionLeft +=
            bandLeft[band] *
            reduction_[band];

        correctionRight +=
            bandRight[band] *
            reduction_[band];
    }

    const double strength=std::clamp(std::isfinite(amount)?amount:0.0,0.0,1.0);
    // Protect initial attack (~18 ms), then allow the resonance tail
    // to be corrected; fall to zero after 360 ms without a new onset.
    const double gate=samplesSinceOnset_>transientDelaySamples_ &&
        samplesSinceOnset_<onsetHoldSamples_ ? 1.0 : 0.0;
    left -= strength*gate*correctionLeft;
    right -= strength*gate*correctionRight;
}

double
AdaptiveResonanceSuppressor::currentMaximumReduction()
    const noexcept {

    double maximum = 0.0;

    for (const double value :
         reduction_) {
        maximum =
            std::max(
                maximum,
                value);
    }

    return maximum;
}

double
AdaptiveResonanceSuppressor::primaryFrequency()
    const noexcept {

    std::size_t best = 0;

    for (std::size_t band = 1;
         band < kBandCount;
         ++band) {

        if (reduction_[band] >
            reduction_[best]) {
            best = band;
        }
    }

    return kCenters[best];
}

} // namespace a125::drum::dsp
