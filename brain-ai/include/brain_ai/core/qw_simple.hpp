#pragma once
#include <Eigen/Dense>
#include <complex>
#include <cmath>

namespace brain {

using Scalar = double;
using Matrix = Eigen::MatrixXcd;
using Vector = Eigen::VectorXcd;

class QuantumWorkspace {
public:
    explicit QuantumWorkspace(int dim) 
        : dim_(dim), rho_(Matrix::Identity(dim, dim) / static_cast<Scalar>(dim)) {
        rho_ /= rho_.trace();
    }
    
    void step(Scalar dt) {
        // Simple Lindblad evolution
        const Scalar gamma = 0.05;  // Decoherence rate
        
        // Hamiltonian (random for now, should be data-driven)
        Matrix H = Matrix::Random(dim_, dim_);
        H = (H + H.adjoint()) / 2.0;  // Make Hermitian
        
        // von Neumann term
        std::complex<Scalar> i(0.0, 1.0);
        Matrix drho = -i * dt * (H * rho_ - rho_ * H);
        
        // Lindblad dissipation (simple dephasing)
        for (int k = 0; k < dim_; ++k) {
            Matrix Lk = Matrix::Zero(dim_, dim_);
            Lk(k, k) = 1.0;
            drho += gamma * dt * (Lk * rho_ * Lk.adjoint() - 
                                 0.5 * (Lk.adjoint() * Lk * rho_ + rho_ * Lk.adjoint() * Lk));
        }
        
        rho_ += drho;
        
        // Ensure PSD and trace=1
        rho_ = (rho_ + rho_.adjoint()) / 2.0;
        Scalar tr = rho_.trace().real();
        if (tr > 0) rho_ /= tr;
    }
    
    Scalar entropy() const {
        Eigen::SelfAdjointEigenSolver<Matrix> solver(rho_);
        Scalar S = 0.0;
        for (int i = 0; i < dim_; ++i) {
            Scalar lambda = solver.eigenvalues()[i].real();
            if (lambda > 1e-12) {
                S -= lambda * std::log(lambda);
            }
        }
        return S;
    }
    
    Scalar trace() const {
        return rho_.trace().real();
    }
    
    int dimension() const { return dim_; }
    
private:
    int dim_;
    Matrix rho_;
};

} // namespace brain