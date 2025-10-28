#include <gtest/gtest.h>
#include "brain_ai/core/qw_simple.hpp"

TEST(QuantumWorkspaceTest, BasicInitialization) {
    brain::QuantumWorkspace qw(7);
    EXPECT_EQ(qw.dimension(), 7);
    EXPECT_GE(qw.entropy(), 0.0);
    EXPECT_LE(qw.entropy(), std::log(7.0));
}

TEST(QuantumWorkspaceTest, StepExecution) {
    brain::QuantumWorkspace qw(7);
    double initial_entropy = qw.entropy();
    
    qw.step(0.01);
    
    // Entropy should change
    EXPECT_NE(qw.entropy(), initial_entropy);
}

TEST(QuantumWorkspaceTest, TracePreservation) {
    brain::QuantumWorkspace qw(7);
    
    for (int i = 0; i < 100; ++i) {
        qw.step(0.01);
    }
    
    // Trace should remain close to 1
    EXPECT_NEAR(qw.trace(), 1.0, 1e-6);
}
