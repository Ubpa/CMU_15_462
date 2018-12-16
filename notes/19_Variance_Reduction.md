# 19 Variance Reduction 

## 19.1 Sampling Strategies 

**Naïve Path Tracing**

![1544940395413](assets/1544940395413.png)

Naïve path tracing misses important phenomena! 

**Importance Sampling in Rendering**

![1544940446496](assets/1544940446496.png)

**Bidirectional Path Tracing**

Forward path tracing: no control over path length (hits light after n bounces, or gets terminated by Russian Roulette) 

Idea: connect paths from light, eye (“bidirectional”) 

![1544940538425](assets/1544940538425.png)

**Metropolis-Hastings Algorithm**

Good paths can be hard to find

![1544940588563](assets/1544940588563.png)

> perturb 扰乱

 Standard Monte Carlo: sum up independent samples 

MH: take random walk of dependent samples (“mutations”) 

Basic idea: prefer to take steps that increase sample value 

![1544940682640](assets/1544940682640.png)

If careful, sample distribution will be proportional to integrand 

- make sure mutations are “ergodic”  
- need to take a long walk, so initial point doesn’t matter (“mixing”) 

> 关于Metropolis-Hastings Algorithm的更详细内容，可以参考 [wikipedia_MHA](https://en.wikipedia.org/wiki/Metropolis%E2%80%93Hastings_algorithm) 

**Metropolis-Hastings: Sampling an Image**

- Want to take samples proportional to image density f
- Start at random point; take steps in (normal) random direction
- Occasionally jump to random point (ergodicity)
- Transition probability is “relative darkness” f(x’)/f(xi) 

![1544941635673](assets/1544941635673.png)

**Metropolis Light Transport** 

![1544941672031](assets/1544941672031.png)

![1544941682680](assets/1544941682680.png)

**Multiple Importance Sampling (MIS)**

Many possible importance sampling strategies 

Which one should we use for a given integrand? 

MIS: combine strategies to preserve strengths of all of them 

Balance heuristic is (provably!) about as good as anything

![1544941726743](assets/1544941726743.png)

![1544941743354](assets/1544941743354.png)

## 19.2 Sampling Patterns

**Sampling Patterns & Variance Reduction**

Want to pick samples according to a given density

But even for uniform density, lots of possible sampling patterns

Sampling pattern will affect variance (of estimator!) 

![1544941806919](assets/1544941806919.png)

**Stratifed Sampling**

stratifed estimate never has larger variance (often lower) 

![1544941938864](assets/1544941938864.png)

**Low-Discrepancy(差异) Sampling**

“No clumps(块)” hints at one possible criterion for a good sample 

Number of samples should be proportional to area

Discrepancy measures deviation(偏差) from this ideal 

![1544942252426](assets/1544942252426.png)

**Quasi-Monte Carlo methods (QMC)**

Replace truly random samples with low-discrepancy samples 

Why? Koksma’s theorem: 

![1544942285320](assets/1544942285320.png)

**Hammersley & Halton Points**

Can easy generate samples with near-optimal discrepancy 

First defne radical inverse  $\varphi_r(i)$ 

Express integer i in base r, then refect digits around decimal 

> φ10(1234) = 0.4321 

Can get n Halton points $x_1, …, x_n$ in k-dimensions via 
$$
x_i=(\phi_{P_1}(i),\phi_{P_2}(i),...,\phi_{P_k}(i))
$$
Similarly, Hammersley sequence is 
$$
x_i=(i/n,\phi_{P_1}(i),\phi_{P_2}(i),...,\phi_{P_{k-1}}(i))
$$

> n must be known ahead of time! 

![1544942537326](assets/1544942537326.png)

**Adaptive Blue Noise**

Can adjust cell size to sample a given density (e.g., importance) 

![1544942607289](assets/1544942607289.png)

## 19.3 efficiently sample from a large distribution 

**Sampling from the CDF** 

![1544942881191](assets/1544942881191.png)

cost is $O(n\log n)$

> 不理解
>
> 用二分查找法的话只需$O(\log n)$
>
> 建表只需$O(n)$

**Alias Table**

Get amortized O(1) sampling by building “alias table” 

Basic idea: rob from the rich, give to the poor (O(n)): 

![1544943037070](assets/1544943037070.png)

Table just stores two identities & ratio of heights per column 

To sample: 

- pick uniform # between 1 and n
- biased coin fip to pick one of the two identities in nth column 

## 19.4 Other techniques

- Photon Mapping 
- Finite Element Radiosity 
- ...