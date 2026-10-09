#pragma once

#include "Biquad.h"

#include <array>
#include <cstddef>

namespace a125::drum::dsp {

class AdaptiveResonanceSuppressor {
public:
    static constexpr std::size_t kBandCount = 10;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void processFrame(double& left, double& right, double amount=1.0) noexcept;

    double currentMaximumReduction() const noexcept;
    double primaryFrequency() const noexcept;

private:
    static double timeCoefficient(
        double sampleRate,
        double milliseconds) noexcept;

    void updateTargets() noexcept;

    static constexpr std::array<double, kBandCount>
        kCenters {
            180.0, 260.0, 380.0, 550.0, 800.0,
            1200.0, 1800.0, 2700.0, 4100.0, 6500.0
        };

    std::array<std::array<Biquad, kBandCount>, 2> detectors_ {};
    std::array<double, kBandCount> slowEnergy_ {};
    std::array<double, kBandCount> reduction_ {};
    std::array<double, kBandCount> targetReduction_ {};

    double sampleRate_ {44100.0};
    double wideEnergy_ {0.0};

    double energyAttack_ {0.0};
    double energyRelease_ {0.0};
    double wideAttack_ {0.0};
    double wideRelease_ {0.0};
    double reductionAttack_ {0.0};
    double reductionRelease_ {0.0};

    std::size_t updateCounter_ {0};
    double transientFast_ {0.0};
    double transientSlow_ {0.0};
    double transientFastA_ {0.0};
    double transientSlowA_ {0.0};
    std::size_t samplesSinceOnset_ {0};
    std::size_t lastOnset_ {0};
    std::size_t onsetHoldSamples_ {0};
    std::size_t transientDelaySamples_ {0};
    std::size_t refractorySamples_ {0};
};

} // namespace a125::drum::dsp
