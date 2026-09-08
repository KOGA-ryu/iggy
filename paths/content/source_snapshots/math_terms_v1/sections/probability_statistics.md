# Probability and Statistics

## Subcategory Map

1. Probability Foundations
   - Sample Space and Events
   - Axioms and Conditioning
2. Random Variables
   - Discrete Models
   - Continuous Models
3. Distribution Theory
   - Common Distributions
   - Limit Laws
4. Statistical Inference
   - Estimation
   - Hypothesis Testing
5. Stochastic Processes
   - Markov Models
   - Finance and Machine Learning Links

## Table of Contents

1. [Probability Foundations](#probability-foundations)
   1. [Sample Space and Events](#sample-space-and-events)
   2. [Axioms and Conditioning](#axioms-and-conditioning)
2. [Random Variables](#random-variables)
   1. [Discrete Models](#discrete-models)
   2. [Continuous Models](#continuous-models)
3. [Distribution Theory](#distribution-theory)
   1. [Common Distributions](#common-distributions)
   2. [Limit Laws](#limit-laws)
4. [Statistical Inference](#statistical-inference)
   1. [Estimation](#estimation)
   2. [Hypothesis Testing](#hypothesis-testing)
5. [Stochastic Processes](#stochastic-processes)
   1. [Markov Models](#markov-models)
   2. [Finance and Machine Learning Links](#finance-and-machine-learning-links)
6. [Round 1 Supplemental Terms](#round-1-supplemental-terms)
   1. [Foundational Probability and Inference](#foundational-probability-and-inference)
7. [Round 2 Supplemental Terms](#round-2-supplemental-terms)
   1. [Risk and Learning Core Terms](#risk-and-learning-core-terms)
   2. [Decision and Concentration](#decision-and-concentration)
   3. [Estimation and Testing Extensions](#estimation-and-testing-extensions)
8. [Round 3 Supplemental Terms](#round-3-supplemental-terms)
   1. [Bayesian Foundations](#bayesian-foundations)
   2. [Time Series and Risk](#time-series-and-risk)
   3. [Learning Geometry](#learning-geometry)
9. [Round 4 Supplemental Terms](#round-4-supplemental-terms)
   1. [Computational Methods](#computational-methods)
   2. [Estimator Quality](#estimator-quality)
   3. [Dependence and Tail Risk](#dependence-and-tail-risk)
10. [Round 5 Supplemental Terms](#round-5-supplemental-terms)
   1. [Stochastic Calculus Core](#stochastic-calculus-core)
   2. [Mathematical Finance Mechanics](#mathematical-finance-mechanics)
   3. [Inferential Learning for Time-Dependent Data](#inferential-learning-for-time-dependent-data)
11. [Round 6 Supplemental Terms](#round-6-supplemental-terms)
    1. [Decision and Estimation Tradeoffs](#decision-and-estimation-tradeoffs)
    2. [Sequential and Online Learning](#sequential-and-online-learning)
    3. [Modern Risk Measures](#modern-risk-measures)
12. [Round 7 Supplemental Terms](#round-7-supplemental-terms)
    1. [Optimization and Risk Measures](#optimization-and-risk-measures)
    2. [Information and Generalization](#information-and-generalization)
    3. [Financial Time-Series Dynamics](#financial-time-series-dynamics)
13. [Round 8 Supplemental Terms](#round-8-supplemental-terms)
    1. [Detection and Filtering](#detection-and-filtering)
    2. [Robustness and Misspecification](#robustness-and-misspecification)
    3. [Bayesian Computation](#bayesian-computation)
14. [Round 9 Supplemental Terms](#round-9-supplemental-terms)
    1. [Extreme-Value Theory](#extreme-value-theory)
    2. [Optimal Transport and Generative Learning](#optimal-transport-and-generative-learning)
    3. [Stochastic Models for Finance and AI](#stochastic-models-for-finance-and-ai)

## Probability Foundations

### Sample Space and Events

#### Term: Sample Space
A **sample space** is the set of all possible outcomes of an experiment, typically denoted \(\Omega\).

#### Term: Event
An **event** is any subset of \(\Omega\) that is in the associated sigma-algebra.

### Axioms and Conditioning

#### Term: Probability Measure
A function 
\(\mathbb P:\mathcal F\to[0,1]\) with 
\(\mathbb P(\Omega)=1\), nonnegativity, and countable additivity.

#### Term: Conditional Probability
For events 
\(A,B\) with
\(\mathbb P(B)>0\):
\[
\mathbb P(A\mid B)=\frac{\mathbb P(A\cap B)}{\mathbb P(B)}.
\]

#### Term: Bayes' Theorem
For partition 
\((B_i)_{i}\) with 
\(\mathbb P(B_i)>0\),
\[
\mathbb P(B_k\mid A)=\frac{\mathbb P(A\mid B_k)\mathbb P(B_k)}{\sum_i\mathbb P(A\mid B_i)\mathbb P(B_i)}.
\]

## Random Variables

### Discrete Models

#### Term: Random Variable
A measurable map 
\(X:(\Omega,\mathcal F)\to(E,\mathcal E)\), often 
\(\mathbb R\) with Borel sigma-algebra.

#### Term: Probability Mass Function
For discrete 
\(X\), 
\(p_X(x)=\mathbb P(X=x)\) with 
\(\sum_x p_X(x)=1\).

#### Term: Expectation
For discrete 
\(X\), 
\[
\mathbb E[X]=\sum_x x\,p_X(x).
\]

### Continuous Models

#### Term: Probability Density Function
A function 
\(f_X\) for continuous 
\(X\) such that 
\(\mathbb P(a\le X\le b)=\int_a^b f_X(x)\,dx\).

#### Term: Cumulative Distribution Function
\[
F_X(x)=\mathbb P(X\le x).
\]

#### Term: Moment Generating Function
\[
M_X(t)=\mathbb E[e^{tX}],
\]
when finite in neighborhood of 
\(t=0\).

## Distribution Theory

### Common Distributions

#### Term: Bernoulli Distribution
\(X\sim\mathrm{Bern}(p)\) has 
\(\mathbb P(X=1)=p\), 
\(\mathbb P(X=0)=1-p\).

#### Term: Binomial Distribution
\(X\sim\mathrm{Bin}(n,p)\):
\[
\mathbb P(X=k)=\binom{n}{k}p^k(1-p)^{n-k}.
\]

#### Term: Normal Distribution
\(X\sim N(\mu,\sigma^2)\) with density
\[
f_X(x)=\frac1{\sigma\sqrt{2\pi}}\exp\left(-\frac{(x-\mu)^2}{2\sigma^2}\right).
\]

### Limit Laws

#### Term: Law of Large Numbers
If 
\(X_i\) are i.i.d. with mean 
\(\mu\), sample average
\(\bar X_n\to\mu\) in probability.

#### Term: Central Limit Theorem
If 
\(X_i\) i.i.d. with mean 
\(\mu\), variance 
\(\sigma^2\),
\[
\frac{\sum_{i=1}^n X_i-n\mu}{\sigma\sqrt n}\xrightarrow{d}\mathcal N(0,1).
\]

## Statistical Inference

### Estimation

#### Term: Likelihood Function
For data 
\(x\), parameter 
\(\theta\):
\(L(\theta\mid x)=p(x\mid\theta)\).

#### Term: Maximum Likelihood Estimator
An MLE 
\(\hat\theta_{\mathrm{MLE}}\) maximizes 
\(L(\theta\mid x)\) (or log-likelihood).

#### Term: Unbiased Estimator
Estimator 
\(\hat\theta\) is unbiased if
\(\mathbb E[\hat\theta]=\theta\).

### Hypothesis Testing

#### Term: Null and Alternative Hypotheses
Statistical test compares 
\(H_0\) vs 
\(H_1\).

#### Term: p-Value
For test statistic 
\(T\), p-value is the null-tail probability of outcomes at least as extreme as observed.

## Stochastic Processes

### Markov Models

#### Term: Markov Chain
A stochastic process 
\(X_t\) with
\[\mathbb P(X_{t+1}=j\mid X_t=i,X_{t-1},\dots)=\mathbb P(X_{t+1}=j\mid X_t=i).
\]

#### Term: Stationary Distribution
A distribution 
\(\pi\) is stationary if
\(\pi P=\pi\) for transition matrix 
\(P\).

### Finance and Machine Learning Links

#### Term: Brownian Motion
A process 
\((W_t)\) with continuous paths, independent increments, and
\(W_{t+s}-W_t\sim\mathcal N(0,s)\).

#### Term: Ito Integral
For adapted process 
\(X_t\),
\[\int_0^T X_t\,dW_t\]
is defined in mean-square (Itô) sense.

#### Term: Risk-Neutral Probability
A probability measure \(\mathbb Q\) such that discounted asset prices are martingales.

## Round 1 Supplemental Terms

### Foundational Probability and Inference

#### Term: Random Walk
A process 
\(S_n=\sum_{i=1}^n X_i\) for i.i.d. increments 
\(X_i\).

#### Term: Covariance
\[
\mathrm{Cov}(X,Y)=\mathbb E[(X-\mu_X)(Y-\mu_Y)].
\]

#### Term: Correlation
\[
\rho_{XY}=\frac{\mathrm{Cov}(X,Y)}{\sigma_X\sigma_Y},\quad |\rho_{XY}|\le1.
\]

#### Term: Poisson Distribution
\[\mathbb P(N=n)=\frac{\lambda^n e^{-\lambda}}{n!},\quad n\in\mathbb N_0.
\]

#### Term: Entropy
\[
H(X)=-\sum_x p_X(x)\log p_X(x).
\]

## Round 2 Supplemental Terms

### Risk and Learning Core Terms

#### Term: Variance
The variance of \(X\) is
\[
\mathrm{Var}(X)=\mathbb E[(X-\mu)^2],\quad \mu=\mathbb E[X].
\]

#### Term: Standard Deviation
Standard deviation is \(\sigma_X=\sqrt{\mathrm{Var}(X)}\), the scale of spread in same units as \(X\).

#### Term: Covariance Matrix
For \(X\in\mathbb R^d\), covariance matrix is
\[
\Sigma_{ij}=\mathrm{Cov}(X_i,X_j).
\]
\(\Sigma\) is symmetric and positive semidefinite.

#### Term: Characteristic Function
The characteristic function of \(X\) is
\[
\varphi_X(t)=\mathbb E[e^{itX}],
\]
which uniquely determines the distribution.

### Decision and Concentration

#### Term: Conditional Expectation
Conditional expectation \( \mathbb E[X\mid\mathcal G] \) is the \( \mathcal G \)-measurable random variable minimizing mean squared error among all \(\mathcal G\)-measurable \(Y\):
\[
\mathbb E\!\left[(X-Y)^2\right]\text{ is minimized by }Y=\mathbb E[X\mid\mathcal G].
\]

#### Term: Law of Total Probability
For disjoint events \(B_i\) partitioning \(\Omega\):
\[
\mathbb P(A)=\sum_i \mathbb P(A\mid B_i)\mathbb P(B_i).
\]

#### Term: Chebyshev Inequality
If \(X\) has finite mean \(\mu\) and variance \(\sigma^2\), then for \(k>0\):
\[
\mathbb P\big(|X-\mu|\ge k\big)\le \frac{\sigma^2}{k^2}.
\]

### Estimation and Testing Extensions

#### Term: Confidence Interval
A \((1-\alpha)\)-level confidence interval for \(\theta\) is a random interval \([L(X),U(X)]\) with
\[
\mathbb P_\theta\!\left(L(X)\le \theta\le U(X)\right)\ge 1-\alpha.
\]

#### Term: Convergence in Distribution
Sequence \(X_n\) converges in distribution to \(X\), written \(X_n\xrightarrow{d}X\), if \(\mathbb E[f(X_n)]\to\mathbb E[f(X)]\) for all bounded continuous \(f\).

#### Term: Kullback-Leibler Divergence
For densities \(p,q\):
\[
D_{\mathrm{KL}}(p\|q)=\int p(x)\log\frac{p(x)}{q(x)}\,dx.
\]
It measures directed divergence and is nonnegative.

## Round 3 Supplemental Terms

### Bayesian Foundations

#### Term: Prior Distribution
The prior \(p(\theta)\) expresses belief about parameter \(\theta\) before observing data.

#### Term: Posterior Distribution
Posterior updates prior using data \(x\):
\[
p(\theta\mid x)\propto p(x\mid\theta)\,p(\theta).
\]

#### Term: Maximum A Posteriori
MAP estimate:
\[
\hat\theta_{\mathrm{MAP}}=\arg\max_\theta p(\theta\mid x).
\]
It is the mode of posterior \(p(\theta\mid x)\).

### Time Series and Risk

#### Term: Stationary Process
A stochastic process \((X_t)\) is (weakly) stationary when its mean is constant and covariance depends only on lag.

#### Term: Autocovariance
For lag \(h\):
\[
\gamma(h)=\mathrm{Cov}(X_t,X_{t+h}).
\]

#### Term: Autoregressive Process
AR(1):
\[
X_t=\phi X_{t-1}+\epsilon_t,\qquad \epsilon_t\sim\text{white noise}.
\]

#### Term: Value at Risk
For loss \(L\), confidence \(1-\alpha\), VaR is:
\[
\mathrm{VaR}_{\alpha}=\inf\{x:\mathbb P(L\le x)\ge 1-\alpha\}.
\]

#### Term: Conditional Value at Risk
\[
\mathrm{CVaR}_{\alpha}=\mathbb E[L\mid L\ge \mathrm{VaR}_{\alpha}]
\]
is the expected loss beyond VaR.

### Learning Geometry

#### Term: Log-Likelihood
Log-likelihood for i.i.d. data \(x_i\):
\[
\ell(\theta)=\sum_i \log p(x_i\mid\theta).
\]

#### Term: Fisher Information
For scalar \(\theta\):
\[
\mathcal I(\theta)=\mathbb E\!\left[\left(\frac{\partial}{\partial\theta}\log p(X\mid\theta)\right)^2\right].
\]

#### Term: Cramér-Rao Lower Bound
For unbiased \(\hat\theta\),
\[
\mathrm{Var}(\hat\theta)\ge \frac{1}{\mathcal I(\theta)}.
\]

#### Term: Bregman Divergence
For convex \(\phi\):
\[
D_\phi(p\|q)=\phi(p)-\phi(q)-\phi'(q)(p-q).
\]
## Round 4 Supplemental Terms

### Computational Methods

#### Term: Monte Carlo Estimator
For \(\mathbb E[f(X)]\), with \(X_1,\dots,X_n\) i.i.d.,
\[
\hat\mu_n=\frac1n\sum_{i=1}^n f(X_i)
\]
is unbiased if \( \mathbb E[f(X)]\) exists and converges by LLN.

#### Term: Importance Sampling
To estimate \(\mathbb E_p[g(X)]\) using proposal \(q\),
\[
\mathbb E_p[g(X)] = \mathbb E_q\!\left[g(X)\frac{p(X)}{q(X)}\right].
\]
Variance improves when \(q\) places more mass where \(g(X)p(X)\) is large.

#### Term: Markov Chain Monte Carlo
**MCMC** builds a Markov chain with stationary law \(\pi\) so samples are asymptotically distributed as \(\pi\), enabling expectations by averages.

#### Term: Ergodicity (Markov Chains)
A chain is ergodic when it has a unique stationary distribution and converges to it from any start, typically via irreducible and aperiodic behavior.

### Estimator Quality

#### Term: Bias
Bias of estimator \(\hat\theta\) for \(\theta\):
\[
\operatorname{Bias}(\hat\theta)=\mathbb E[\hat\theta]-\theta.
\]

#### Term: Mean Squared Error
\[
\operatorname{MSE}(\hat\theta)=\mathbb E[(\hat\theta-\theta)^2]
=\operatorname{Var}(\hat\theta)+\operatorname{Bias}(\hat\theta)^2.
\]

#### Term: Consistency
An estimator \(\hat\theta_n\) is consistent for \(\theta\) if
\[
\hat\theta_n\xrightarrow{P}\theta.
\]

#### Term: Law of the Unconscious Statistician
For measurable \(g\) and \(X\),
\[
\mathbb E[g(X)]=\int g(x)\,dF_X(x)=\int g(x)f_X(x)\,dx.
\]

### Dependence and Tail Risk

#### Term: Copula
A copula \(C\) couples marginal CDFs \(F_1,\dots,F_d\) into a joint CDF:
\[
F(x_1,\dots,x_d)=C(F_1(x_1),\dots,F_d(x_d)).
\]

#### Term: Tail Dependence
Upper-tail dependence \(\lambda_U\) between \(X,Y\):
\[
\lambda_U=\lim_{u\uparrow1}\mathbb P\!\left(Y>F_Y^{-1}(u)\mid X>F_X^{-1}(u)\right).
\]

#### Term: Student's \(t\)-Copula
The \(t\)-copula uses a multivariate \(t\)-distribution template to model symmetric heavy-tail dependence.

#### Term: Expected Shortfall (Alternate Form)
For loss \(L\) and level \(\alpha\):
\[
\mathrm{ES}_\alpha = \frac{1}{\alpha}\int_0^\alpha \mathrm{VaR}_u\,du
\]
for \(\alpha\in(0,1)\).

## Round 5 Supplemental Terms

### Stochastic Calculus Core

#### Term: Martingale
A process \((M_t)\) adapted to filtration \((\mathcal F_t)\) is a martingale if
\[
\mathbb E[|M_t|]<\infty,\quad \mathbb E[M_t\mid\mathcal F_s]=M_s,\ \forall s\le t.
\]

#### Term: Quadratic Variation
For semimartingale \(X\), quadratic variation up to \(T\) is
\[
[X]_T=\lim_{\|\pi\|\to0}\sum_{i}(X_{t_i}-X_{t_{i-1}})^2.
\]

#### Term: It\^o Process
An It\^o process has form
\[
dX_t=\mu_t\,dt+\sigma_t\,dW_t.
\]

#### Term: Stochastic Integral
For adapted \(X_t\) and Brownian motion \(W_t\),
\[
\int_0^T X_t\,dW_t
\]
is defined as an \(L^2\) limit of left-point Riemann sums.

### Mathematical Finance Mechanics

#### Term: Geometric Brownian Motion
GBM satisfies
\[
dS_t=\mu S_t\,dt+\sigma S_t\,dW_t,\quad S_0>0.
\]

#### Term: Risk-Neutral Measure
Under \(\mathbb Q\), discounted asset prices become martingales and expected growth equals the risk-free rate.

#### Term: Girsanov Theorem
Girsanov changes drift via a measure shift; under suitable Novikov conditions, Brownian motion under \(\mathbb P\) becomes Brownian with drift removed under \(\mathbb Q\).

#### Term: Black–Scholes PDE
For option value \(V(t,S)\):
\[
\frac{\partial V}{\partial t}
+\frac12\sigma^2S^2\frac{\partial^2V}{\partial S^2}
+rS\frac{\partial V}{\partial S}
-rV=0.
\]

### Inferential Learning for Time-Dependent Data

#### Term: Filtered Probability Space
An increasing family of sigma-algebras \((\mathcal F_t)_{t\ge0}\) represents information available up to time \(t\).

#### Term: Radon--Nikodym Derivative
If \(Q\ll P\), then
\[
\frac{dQ}{dP}
\]
is the density giving \(Q(A)=\int_A \frac{dQ}{dP}\,dP\).

#### Term: Cross-Entropy
Cross-entropy of \(p\) relative to \(q\):
\[
H(p,q)=-\sum_x p(x)\log q(x).
\]

#### Term: KL to Wasserstein Link
KL controls distribution proximity, while Wasserstein captures geometry/transport; both are used in generative model objectives.

## Round 6 Supplemental Terms

### Decision and Estimation Tradeoffs

#### Term: Bayes Risk
Bayes risk under decision rule \(\delta\) and prior \(\pi\) is the posterior-weighted expected loss:
\[
r_B(\delta)=\mathbb E_{\theta\sim\pi}\,\mathbb E_{X\mid\theta}\left[\ell\!\left(\delta(X),\theta\right)\right].
\]

#### Term: Expected Utility
An action is chosen by maximizing expected utility:
\[
\max_a \mathbb E[U(a,X)].
\]

#### Term: Regret
For action \(a\), expected regret is
\[
\mathrm{Regret}(a)=\mathbb E[L(a,X)]-\inf_{a'}\mathbb E[L(a',X)].
\]

### Sequential and Online Learning

#### Term: Markov Decision Process
An MDP is the tuple \((\mathcal S,\mathcal A,P,r,\gamma)\):
state space, action space, transition kernel, reward, and discount factor.

#### Term: Bellman Equation
For policy \(\pi\) with value function \(V^\pi\),
\[
V^\pi(s)=\mathbb E\!\left[r(s,a)+\gamma V^\pi(S')\mid S=s,A\sim\pi(\cdot\mid s)\right].
\]

#### Term: Q-Value Function
The action-value function is
\[
Q^\pi(s,a)=\mathbb E\!\left[r(s,a)+\gamma\sum_{a'}\pi(a'\mid S')Q^\pi(S',a')\mid S=s,A=a\right].
\]

#### Term: Temporal Difference Error
At step \(t\), TD error is
\[
\delta_t=R_{t+1}+\gamma V(S_{t+1})-V(S_t).
\]

### Modern Risk Measures

#### Term: Drawdown
For cumulative value \(V_t\), drawdown is
\[
\mathrm{DD}_t=\max_{0\le s\le t}V_s-V_t.
\]

#### Term: Maximum Drawdown
Maximum drawdown over horizon \(T\):
\[
\mathrm{MDD}=\max_{0\le t\le T}\mathrm{DD}_t.
\]

#### Term: Sharpe Ratio
For return mean \(\mu\), risk-free rate \(r_f\), and volatility \(\sigma\),
\[
\mathrm{SR}=\frac{\mu-r_f}{\sigma}.
\]

#### Term: Calmar Ratio
Calmar ratio is return normalized by max drawdown:
\[
\mathrm{Calmar}=\frac{\text{Annualized Return}}{\mathrm{MDD}}.
\]

## Round 7 Supplemental Terms

### Optimization and Risk Measures

#### Term: Convex Risk Measure
A map \(\rho:L^\infty\to\mathbb R\) is a convex risk measure when it satisfies monotonicity, translation invariance, and convexity in losses.

#### Term: Coherent Risk Measure
A coherent risk measure is a convex risk measure additionally satisfying positive homogeneity and subadditivity.
\[
\rho(X+Y)\le\rho(X)+\rho(Y),\qquad \rho(\lambda X)=\lambda\rho(X),\;\lambda\ge0.
\]

#### Term: Entropic Risk Measure
For \(\theta>0\),
\[
\rho_\theta(X)=\frac1\theta\log\mathbb E[e^{\theta X}]
\]
penalizes tail events with exponential weighting.

#### Term: Conditional Value at Risk as Optimization
CVaR admits dual form
\[
\mathrm{CVaR}_\alpha(X)=\min_{\eta\in\mathbb R}\left\{\eta+\frac1\alpha\mathbb E[(X-\eta)_+]\right\}.
\]

#### Term: Mean-Variance Frontier
The set of admissible portfolios \((\mu,\sigma^2)\) generated by
\[
\min_w w^\top\Sigma w\quad\text{subject to}\quad \mu^\top w=\mu_0,\; \mathbf 1^\top w=1.
\]
defines the efficient frontier.

### Information and Generalization

#### Term: PAC-Bayesian Bound
For posterior \(Q\), prior \(P\), empirical risk \(\hat L\), true risk \(L\), and \(n\) samples:
\[
L(Q)\le \hat L(Q)+\sqrt{\frac{\mathrm{KL}(Q\|P)+\log(2\sqrt n/\delta)}{2(n-1)}}.
\]

#### Term: Rademacher Complexity
For class \(\mathcal F\), sample \(x_1,\dots,x_n\), and Rademacher signs \(\sigma_i\):
\[
\hat{\mathfrak R}_n(\mathcal F)=\mathbb E_\sigma\left[\sup_{f\in\mathcal F}\frac1n\sum_{i=1}^n \sigma_i f(x_i)\right].
\]

#### Term: VC Dimension
VC dimension is the largest \(d\) such that some set of \(d\) points is shattered by a hypothesis class.

#### Term: Information Bottleneck Objective
Given encoder \(p(z\mid x)\), objective trades compression and prediction:
\[
\mathcal L=\mathbb E[-\log q(y\mid z)]+\beta\,I(X;Z).
\]

### Financial Time-Series Dynamics

#### Term: ARCH Process
In ARCH(\(q\)):
\[
r_t=\mu+\epsilon_t,\qquad \epsilon_t=\sigma_t z_t,\qquad \sigma_t^2=\alpha_0+\sum_{i=1}^q\alpha_i\epsilon_{t-i}^2.
\]

#### Term: GARCH Process
In GARCH(\(p,q\)):
\[
\sigma_t^2=\alpha_0+\sum_{i=1}^q\alpha_i\epsilon_{t-i}^2+\sum_{j=1}^p\beta_j\sigma_{t-j}^2.
\]

#### Term: Stochastic Volatility
Model with latent log-variance:
\[
h_t=\phi h_{t-1}+\eta_t,\quad r_t=\exp(h_t/2)\, \epsilon_t.
\]

#### Term: Hurst Exponent
The Hurst exponent \(H\in(0,1)\) measures long-range dependence:
\[
H<0.5:\text{mean-reverting},\quad H=0.5:\text{random walk},\quad H>0.5:\text{persistent}.
\]

## Round 8 Supplemental Terms

### Detection and Filtering

#### Term: CUSUM Statistic
For target \(\mu_0\), cumulative sum detection uses
\[
S_t=\sum_{i=1}^t (X_i-\mu_0).
\]
A change is flagged when \(S_t\) crosses decision boundaries.

#### Term: Likelihood Ratio Test
For hypotheses \(H_0,H_1\) and data \(x\):
\[
\Lambda(x)=\frac{\sup_{\theta\in\Theta_0}L(\theta\mid x)}{\sup_{\theta\in\Theta_1}L(\theta\mid x)}.
\]
Reject \(H_0\) when \(\Lambda(x)\) is sufficiently small.

#### Term: Recursive Bayesian Filtering
For latent state \(X_t\) and observation \(Y_t\):
\[
p(x_t\mid y_{1:t})\propto p(y_t\mid x_t)\int p(x_t\mid x_{t-1})p(x_{t-1}\mid y_{1:t-1})dx_{t-1}.
\]

#### Term: Kalman Filter
For linear-Gaussian state-space models, the filter updates posterior state mean and covariance with a prediction step and an innovation correction.

### Robustness and Misspecification

#### Term: Robust Estimator
An estimator is robust if it is not overly sensitive to small amounts of contamination or outliers.

#### Term: Influence Function
For estimator \(T\) at distribution \(F\), influence function is
\[
\mathrm{IF}(z;T,F)=\left.\frac{d}{d\epsilon}T((1-\epsilon)F+\epsilon\Delta_z)\right|_{\epsilon=0}.
\]

#### Term: Huber Loss
\[
\rho_\delta(u)=
\begin{cases}
\frac12u^2,& |u|\le\delta,\\
\delta|u|-\frac12\delta^2,& |u|>\delta.
\end{cases}
\]

#### Term: Adversarial Robustness Margin
The maximum perturbation radius \(\epsilon\) for which model output remains unchanged under \(\|\eta\|_\infty\le\epsilon\).

### Bayesian Computation

#### Term: Hamiltonian Monte Carlo
HMC simulates Hamiltonian dynamics on an expanded state space (position+momentum) to build long-distance MCMC proposals with high acceptance.

#### Term: Gibbs Sampler
Gibbs sampling updates blocks from full conditionals: draw each parameter block from \(p(\theta_i\mid\theta_{-i},x)\).

#### Term: Variational Bayes
VB solves an optimization problem over tractable families \(\mathcal Q\):
\[
q^*=\arg\min_{q\in\mathcal Q}\mathrm{KL}(q(\theta)\|p(\theta\mid x)).
\]

#### Term: Effective Sample Size
For autocorrelated draws, effective sample size is
\[
n_{\mathrm{eff}}=\frac{n}{1+2\sum_{k\ge1}\rho_k}.
\]

## Round 9 Supplemental Terms

### Extreme-Value Theory

#### Term: Extreme-Value Index
For tail cdf \( \bar F(x)=1-F(x) \), the extreme-value index \(\xi\) characterizes tail decay and dictates whether tails are heavy (\(\xi>0\)), light (\(\xi=0\)), or bounded (\(\xi<0\)).

#### Term: Block Maxima
Divide observations into blocks of size \(m\); maxima \(M_j=\max\{X_{(j-1)m+1},\dots,X_{jm}\}\) are used to fit a GEV model.

#### Term: Generalized Extreme Value Distribution
With location \(\mu\), scale \(\sigma>0\), and shape \(\xi\):
\[
G_{\mu,\sigma,\xi}(x)=\exp\left[-\left(1+\xi\frac{x-\mu}{\sigma}\right)^{-1/\xi}\right],\quad 1+\xi\frac{x-\mu}{\sigma}>0.
\]
At \(\xi\to0\), this converges to the Gumbel form \( \exp(-e^{-z}) \) with \(z=(x-\mu)/\sigma\).

#### Term: Pickands–Balkema–de Haan Theorem
For high threshold \(u\), exceedance tails satisfy
\[
\sup_{0\le y\le y_{\max}}\left| \Pr(X-u\le y\mid X>u)-G_{\xi,\beta}(y)\right|\to 0,
\]
where \(G_{\xi,\beta}\) is a Generalized Pareto distribution.

#### Term: Generalized Pareto Distribution
Tail of \(Y=X-u\mid X>u\):
\[
\Pr(Y>y)=\left(1+\frac{\xi y}{\beta}\right)^{-1/\xi},\quad y\ge0,\;1+\frac{\xi y}{\beta}>0.
\]

### Optimal Transport and Generative Learning

#### Term: Wasserstein Distance
For \(p\ge1\), transport cost \(c(x,y)=\|x-y\|^p\):
\[
W_p(\mu,\nu)=\left(\inf_{\pi\in\Pi(\mu,\nu)}\int \|x-y\|^p\,d\pi(x,y)\right)^{1/p}.
\]
\(\Pi(\mu,\nu)\) is the set of couplings with marginals \(\mu,\nu\).

#### Term: Kantorovich-Rubinstein Duality
For \(p=1\):
\[
W_1(\mu,\nu)=\sup_{\|f\|_{\mathrm{Lip}}\le1}\left(\int f\,d\mu-\int f\,d\nu\right).
\]

#### Term: Maximum Mean Discrepancy
With kernel \(k\) and feature map in RKHS \(\mathcal H\):
\[
\mathrm{MMD}^2(P,Q)=\mathbb E_{x,x'\sim P}k(x,x')+\mathbb E_{y,y'\sim Q}k(y,y')-2\mathbb E_{x\sim P,y\sim Q}k(x,y).
\]

#### Term: Entropic Regularized Optimal Transport
The entropy-regularized objective is
\[
\min_{\pi\in\Pi(P,Q)}\int c(x,y)\,d\pi(x,y)+\varepsilon\,\mathrm{KL}(\pi\|P\otimes Q).
\]
This formulation is used in differentiable transport matching (Sinkhorn-type algorithms).

### Stochastic Models for Finance and AI

#### Term: Lévy Process
A Lévy process \((L_t)\) has stationary independent increments, càdlàg paths, and characteristic exponent \(\psi\) with
\[
\mathbb E[e^{iuL_t}]=e^{t\psi(u)}.
\]

#### Term: Jump-Diffusion Model
An asset model with jumps has SDE
\[
dS_t=\mu S_{t-}\,dt+\sigma S_{t-}\,dW_t+S_{t-}\,dJ_t,
\]
where \(J_t\) is a pure-jump process (often compound Poisson).

#### Term: Variance Gamma Process
VG can be represented as Brownian motion with drift evaluated at random gamma time:
\[
X_t=\theta G_t+\sigma W_{G_t},\qquad G_t\sim\mathrm{Gamma}(t,\nu).
\]

#### Term: Policy Gradient Theorem
For return \(J(\theta)=\mathbb E_\theta\!\left[\sum_t r_t\right]\):
\[
\nabla_\theta J(\theta)=
\mathbb E_\theta\!\left[\sum_t\nabla_\theta\log\pi_\theta(A_t\mid S_t)\,G_t\right].
\]
