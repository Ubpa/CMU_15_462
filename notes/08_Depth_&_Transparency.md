# 08 Depth & Transparency 

## 8.1 Occlusion

**Sampling Depth**

Assume we have a triangle given by:
– the projected 2D coordinates (xi,yi) of each vertex
– the “depth” of each vertex (i.e., distance from the viewer) 

> Q: How do we compute the depth d at a given sample point (x,y)?
> A: ==Interpolate it using barycentric coordinates== (just like any other attribute that varies linearly over the triangle) 

**The depth-buffer(Z-buffer)**

For each coverage sample point, depth-buffer stores the depth of the closest triangle seen so far. 

work with interpenetrating surfaces and supersampling

## 8.2 Compositing

**Representing opacity as alpha **

Alpha describes the opacity of an object 

![1544533874164](assets/1544533874164.jpg)

Alpha: additional channel of image (RGBA) 

**Over operator**

Composite image B with opacity $\alpha_B$ ==over== image A with opacity $\alpha_A$ 

![1544533974824](assets/1544533974824.jpg)
$$
A over B != B over A
$$
"Over" is not commutative

> 最简单的例子：不透明物体在透明物体前与后是完全不同的

**non-premultiplied alpha **
$$
\begin{aligned}
A&=(R_A,G_A,B_A)\\
B&=(R_B,G_B,B_B)\\
C&=\alpha_B B+(1-\alpha_B)\alpha_A A\\
\end{aligned}
$$
**premultiplied alpha**
$$
\begin{aligned}
A'&=\alpha_A A\\
B'&=\alpha_B B\\
C'&=B'+(1-\alpha_B)A'\\
\alpha_C&=\alpha_B+(1-\alpha_B)\alpha_A
\end{aligned}
$$
Notice premultiplied alpha composites alpha just like how it composites rgb. 

Non-premultiplied alpha composites alpha differently than rgb. 

**Problems with non premultiplied alpha**

- Fringe

![1544536449549](assets/1544536449549.jpg)

if we instead use the premultiplied "over" operation, we get the correct alpha

![1544536485099](assets/1544536485099.jpg)

- Pre-filtering



![1544535731146](assets/1544535731146.jpg)

> **Non-premultiplied**
>
> filtered (color,alpha)= (0.5, 0.5, 0, 0.5)，rst color over white = (0.75, 0.75, 0.5)，多了红色
>
> 本来应该不产生影响（因为$\alpha=0$ ）的红底色在滤波的时候跟绿色混合了
>
> **Premultiplied**
>
> filtered (color,alpha)= (0, 0.5, 0, 0.5)，rst color over white = (0.5, 0.75, 0.5)，正常

- Applying “over” repeatedly 

Non-premultiplied alpha is not closed under composition 
$$
\begin{aligned}
A&=(R_A,G_A,B_A)\\
B&=(R_B,G_B,B_B)\\
C&=\alpha_B B+(1-\alpha_B)\alpha_A A\\
\alpha_C&=\alpha_B+(1-\alpha_B)\alpha_A
\end{aligned}
$$
考虑A和B为透明度0.5的红色，则 C=[0.75,0,0], $\alpha_C=0.75$ ，C 变成了 premultiplied alpha，即说明 Non-premultiplied alpha is not closed

> 正确的C的计算如下
>
> $A'=(0.5,0,0)$
>
> $B'=(0.5,0,0)$
>
> $C'=(0.75,0,0),\ \alpha_C=(0.75,0,0)$
>
> $C=C'/\alpha_C=(1.0,0,0)$

**Summary: advantages of premultiplied alpha **

- Simple: compositing operation treats all channels (RGB and A) the same 

- More efficient than non-premultiplied representation: “over” requires fewer math ops 

- Closed under composition 

- Better representation for fltering (upsampling/downsampling) textures with alpha channel 