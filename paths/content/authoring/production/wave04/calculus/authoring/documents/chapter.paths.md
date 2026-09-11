@paths 1
@subject calculus | Calculus
@chapter topic_0063 | Integration Techniques

@lesson prod04_calc_substitution_r | Integration by substitution
@template lesson.v2
@block introduction | start | 1.1 | Reverse a composite derivative
@prose Substitution replaces an inner expression by a new variable when its derivative also appears, possibly with a missing constant factor. Keep the original integrand visible: the final derivative must reproduce that entire function, not just its outer part. You will need polynomial differentiation, rational arithmetic and the elementary derivatives listed below. Trigonometric inputs are in radians.
@endblock
@block definition | terms | 1.2 | Functions and coordinates
@prose The integrand p(x) is the function inside an integral, and x is its original variable. An antiderivative F is a differentiable function satisfying F'(x)=p(x) throughout the stated interval. A prime denotes differentiation. A substitution u=g(x) names an inner function g with a new variable u; f is the outer function, so f(g(x)) means evaluate f at the output of g. The multiplier m(x) is the remaining factor in the integrand.
@prose The differential relation du=g'(x)dx records the chain-rule factor. It licenses replacing a whole product g'(x)dx by du in an integral; it is not permission to erase unrelated factors. Here m(x)=lambda times g'(x), where lambda is a fixed nonzero real number. H is a primitive of the fully scaled integrand in u, and P(x)=H(g(x)) is its composition back in x. The letter C denotes an arbitrary real constant independent of x and u. The notation R denotes the real numbers.
@display pair
p(x)=m(x)f(g(x)),\qquad m(x)=\lambda g'(x),\qquad u=g(x),\qquad du=g'(x)dx
@prose An indefinite integral describes a constant family P+C. A fixed extra constant, or a nonzero rescaling of arbitrary C, gives the same family. On a connected interval, any two primitives differ by one constant; disconnected components could have independent constants. A definite integral I is instead a signed number. Its displayed lower and upper limits a and b are ordered positions, not instructions to sort numbers by size.
@endblock
@block proposition | rule | 1.3 | Convert every factor
@display substitution
\int m(x)f(g(x))\,dx=\lambda\int f(u)\,du=H(u)+C=H(g(x))+C
@prose First identify g and differentiate it. Match m against g' to obtain lambda. After conversion, the integrand and differential use u only; an x left over indicates an incomplete conversion. Integrate in u, then return to x for an indefinite answer. Here e is the base of the natural exponential and ln is the natural logarithm. The power rule below is used for nonnegative integer n only. The reciprocal case has its own rule, not division by n+1 with n=-1.
@display primitives
\int u^n\,du=\frac{u^{n+1}}{n+1}+C,\qquad \int e^u\,du=e^u+C
@display trigonometric
\int\cos(u)\,du=\sin(u)+C,\qquad\int\sin(u)\,du=-\cos(u)+C
@display logarithm
\int\frac{1}{u}\,du=\ln|u|+C,\qquad u\ne0
@help proof
@prose The chain rule says the derivative of H(g(x)) is H'(g(x)) times g'(x). Since H'(u)=lambda f(u), the derivative becomes lambda f(g(x))g'(x)=m(x)f(g(x))=p(x), an identity on the stated domain. For u nonzero, differentiating ln|u| gives 1/u on either side of zero. The elementary derivatives are (e^u)'=e^u, (sin u)'=cos u, and (cos u)'=-sin u. These identities and polynomial differentiation establish the formulas, rather than sample inputs. If F and P both have derivative p on a connected interval, the mean value theorem applied to F-P makes their difference constant. Thus P+C is the complete family.
@endblock
@block proposition | condition | 1.4 | Domains and transformed limits
@prose A logarithmic primitive needs a nonzero inner argument throughout the stated interval. Absolute value permits negative arguments of g, but does not allow g=0 or an integral crossing a pole. For example, when x<0, 11x-9 is strictly less than -9, so ln|11x-9| is defined whereas ln(11x-9) is not real. Multiplying or cancelling symbols must not enlarge the original domain.
@prose For a definite integral, assume p is continuous on the closed interval between a and b and the chosen primitive is valid there. Replace the bound x=a by u=g(a) and x=b by u=g(b). Preserve their order even if g is decreasing. Use those u-bounds with H, or the original x-bounds with P; never mix them. The Fundamental Theorem of Calculus evaluates upper displayed endpoint minus lower displayed endpoint. No arbitrary C remains because its endpoint values cancel.
@display bounds
I=\int_a^b p(x)\,dx=\lambda\int_{g(a)}^{g(b)}f(u)\,du=H(g(b))-H(g(a))=P(b)-P(a)
@prose A zero of g' alone does not invalidate a substitution identity. Match the differential products without dividing pointwise by g': x dx=du/2 when u=x squared holds even at x=0. Reversing integration limits negates a signed integral. In particular a negative scale with reversed u-limits can produce a positive original integral.
@endblock
@block example | worked | 1.5 | One function, two requests
@prose Find all antiderivatives of p on R and evaluate I. Keep the two requested objects separate.
@display worked_given
p(x)=2x\sin(x^2+2),\qquad I=\int_0^1p(x)\,dx
@help hint
@prose Name the complete sine argument and compare its derivative with the multiplier. For the definite request, apply that same substitution separately to each endpoint before evaluating.
@help answer
@display worked_answer
F(x)=-\cos(x^2+2)+C,\qquad I=\cos(2)-\cos(3)
@help solution
@prose Set u=x squared+2. Then du=2x dx, exactly the full multiplier and differential, so lambda=1. The converted indefinite integral is the integral of sin(u) du. Its primitive is H(u)=-cos(u), since differentiating -cos(u) gives sin(u). Returning to x gives the family below.
@display worked_family
u=x^2+2,\quad du=2x\,dx,\quad H(u)=-\cos(u),\quad F(x)=-\cos(x^2+2)+C
@prose The chain rule gives F'(x)=sin(x squared+2) times 2x, exactly p(x). This identity holds at zero as well. Since R is connected, the constant-difference theorem proves that the family contains every antiderivative. For I, x=0 maps to u=0 squared+2=2; x=1 maps to u=1 squared+2=3. Thus the integral is from 2 to 3 of sin(u) du, not from 0 to 1 of sin(u) du.
@display worked_value
I=[-\cos(u)]_2^3=-\cos(3)-(-\cos(2))=\cos(2)-\cos(3)
@prose Independently in the original coordinate, P(x)=-cos(x squared+2) has P(1)=-cos(3) and P(0)=-cos(2). Subtracting gives the same result. The original integrand is continuous on [0,1]; on this interval its sine argument lies between 2 and 3 radians, where sine is positive, so the signed result is positive. No logarithmic pole or arbitrary constant affects this evaluation.
@endblock
@block example | errors | 1.6 | Diagnose the missing factor
@prose Copying an outer primitive without its scale gives an extra g' in the derivative. A negative g' needs a negative scale; the minus sign from integrating sine is an additional, separate sign. Leaving H in u does not finish an indefinite answer in x. Omitting C gives one primitive rather than the full family. For a reciprocal, a power-zero primitive has derivative zero, not 1/u. In a definite integral, using a and b directly as u-bounds may evaluate the wrong interval even when the primitive is correct. Check the original derivative, the domain and both endpoints independently.
@endblock
@block exercise | practice | 1.7 | Follow the whole substitution
@prose Begin with four linear inner functions, then practise nonlinear pairs and logarithmic domains. Finish by choosing and carrying through substitutions in four definite integrals. Every route checks the original function after integrating; the last four also check the original endpoint order.
@endblock
@block summary | summary | 1.8 | Identity first, then the requested object
@prose Match a complete function-derivative pair, retain its scalar and convert every coordinate. Differentiate the returned x-function to prove the identity. On a connected domain, add arbitrary C for all antiderivatives. For a definite integral, verify continuity, transform both bounds and subtract in displayed order; a family and a signed number are different answers.
@prose Adapted from Matthew Boelkins, with David Austin and Steven Schlicker, Active Calculus Activities Workbook Chapters 5-8, 2018 edition updated August 1, 2024, Section 5.3. Source: https://activecalculus.org/wp-content/uploads/2024/08/acs-activity-workbook-58-2024.pdf . This adapted lesson and its twelve questions are licensed CC BY-SA 4.0: https://creativecommons.org/licenses/by-sa/4.0/ . Some integrands are retained; others have simplified inner functions, exponents, multipliers or new limits. Teaching, choices and feedback are newly written. No figures or photographs are reused. This credit concerns the adapted content, not the application.
@endblock
@practice prod04_calc_substitution_q01
@practice prod04_calc_substitution_q02
@practice prod04_calc_substitution_q03
@practice prod04_calc_substitution_q04
@practice prod04_calc_substitution_q05
@practice prod04_calc_substitution_q06
@practice prod04_calc_substitution_q07
@practice prod04_calc_substitution_q08
@practice prod04_calc_substitution_q09
@practice prod04_calc_substitution_q10
@practice prod04_calc_substitution_q11
@practice prod04_calc_substitution_q12
@end

@question prod04_calc_substitution_q01 | An exponential integral
@template choices.v1
@version 1
@goal Find all antiderivatives of p in x and check the original derivative.
@given p(x)=e^{3x}
@domain Work on R. C is an arbitrary real constant; H is a primitive in u.
@read prod04_calc_substitution_r
@step 10 | Choose the complete exponential argument as u.
@choice 11 | u=3x
@choice 12 | u=x
@choice 13 | u=3x+1
@answer 11
@feedback 12 | This is a legal identity substitution, but leaves the argument as 3u. The requested complete argument is 3x, not x.
@feedback 13 | Adding 1 changes the argument: 3x would become u-1, not u.
@after u=3x
@wrong Name exactly the expression in the exponent.
@why The outer function is the exponential, and its entire input is 3x. With u=3x it becomes e to the u.
@step 20 | Differentiate that substitution to obtain du.
@choice 22 | du=\frac{1}{3}\,dx
@choice 21 | du=3\,dx
@choice 23 | du=-3\,dx
@answer 21
@feedback 22 | The derivative of 3x is 3. The factor 1/3 belongs in dx=du/3, not in du itself.
@feedback 23 | The coefficient of x is positive 3, so differentiation does not introduce a minus sign.
@after du=3\,dx
@wrong Differentiate g before rearranging its differential relation.
@why Differentiating 3x gives 3, hence du=3 dx and dx=du/3.
@step 30 | Convert the whole integral to u, retaining its scale.
@choice 32 | \int p(x)\,dx=\int e^u\,du
@choice 33 | \int p(x)\,dx=3\int e^u\,du
@choice 31 | \int p(x)\,dx=\frac{1}{3}\int e^u\,du
@answer 31
@feedback 32 | This replaces dx by du, but du=3 dx. The converted integral needs the missing factor 1/3.
@feedback 33 | Multiplying by 3 reverses the rearrangement. Since dx=du/3, the multiplier is 1/3, not 3.
@after \int p(x)\,dx=\frac{1}{3}\int e^u\,du
@wrong Account for the differential as well as the exponent.
@why Replacing 3x by u changes the outer function; replacing dx by du/3 supplies the factor 1/3.
@step 40 | Choose a primitive H of the fully scaled u-integrand.
@choice 41 | H(u)=\frac{1}{3}e^u
@choice 42 | H(u)=e^u
@choice 43 | H(u)=-\frac{1}{3}e^u
@answer 41
@feedback 42 | Its u-derivative is e to the u, three times the converted integrand.
@feedback 43 | Its u-derivative is negative e to the u divided by 3; the converted integrand is positive e to the u divided by 3.
@after H(u)=\frac{1}{3}e^u
@wrong The exponential is its own derivative; keep the scalar unchanged.
@why The derivative with respect to u of e to the u divided by 3 is exactly e to the u divided by 3.
@step 50 | Return to x and include the complete constant family.
@choice 52 | F(x)=\frac{1}{3}e^{3x}
@choice 51 | F(x)=\frac{1}{3}e^{3x}+C
@choice 53 | F(x)=e^{3x}+C
@answer 51
@feedback 52 | This gives only one primitive. Adding 1, or any other real constant, gives another that must be included.
@feedback 53 | Back-substitution does not remove the 1/3 scale. This family's derivative is 3 times e to the 3x.
@after F(x)=\frac{1}{3}e^{3x}+C
@wrong Replace u by 3x without changing the scalar, and include arbitrary C.
@why Substituting u=3x into H and adding C gives all constant shifts of the checked primitive.
@step 60 | Differentiate the returned family against the original p.
@choice 62 | F'(x)=\frac{1}{3}e^{3x}
@choice 63 | F'(x)=3e^{3x}
@choice 61 | F'(x)=e^{3x}
@answer 61
@feedback 62 | This omits the inner derivative 3; multiplying 1/3 by 3 restores coefficient 1.
@feedback 63 | The primitive already has coefficient 1/3, so the chain factor 3 cancels it instead of remaining.
@after F'(x)=e^{3x}
@wrong Include the derivative of the exponent; C contributes zero.
@why The chain coefficient is (1/3) times 3=1, exactly the original p. On connected R, every other primitive differs by a constant, so the family is complete.
@end

@question prod04_calc_substitution_q02 | A trigonometric integral
@template choices.v1
@version 1
@goal Find all antiderivatives of p in x and check the original derivative.
@given p(x)=\cos(5x+1)
@domain Work on R. C is an arbitrary real constant; H is a primitive in u.
@read prod04_calc_substitution_r
@step 10 | Choose the complete cosine argument as u.
@choice 12 | u=5x
@choice 11 | u=5x+1
@choice 13 | u=x
@answer 11
@feedback 12 | The +1 belongs to the cosine argument. Omitting it leaves cos(u+1), not cos(u).
@feedback 13 | An identity substitution is legal but leaves cos(5u+1), so it misses the requested complete-argument form.
@after u=5x+1
@wrong Include the constant inside the cosine.
@why The outer cosine receives the whole expression 5x+1, which becomes u.
@step 20 | Differentiate the chosen u.
@choice 22 | du=6\,dx
@choice 23 | du=\frac{1}{5}\,dx
@choice 21 | du=5\,dx
@answer 21
@feedback 22 | The derivative of the constant 1 is zero, so the coefficient is 5+0=5, not 6.
@feedback 23 | Differentiation gives 5; its reciprocal appears only when writing dx=du/5.
@after du=5\,dx
@wrong Differentiate the linear and constant terms separately.
@why The derivative of 5x+1 is 5, so du=5 dx and the full scale is 1/5.
@step 30 | Convert the entire integral to u.
@choice 31 | \int p(x)\,dx=\frac{1}{5}\int\cos(u)\,du
@choice 32 | \int p(x)\,dx=\int\cos(u)\,du
@choice 33 | \int p(x)\,dx=-\frac{1}{5}\int\cos(u)\,du
@answer 31
@feedback 32 | The differential contributes 1/5; without it this is five times the original integral's derivative.
@feedback 33 | Both the multiplier and inner derivative are positive, so their ratio is +1/5, not -1/5.
@after \int p(x)\,dx=\frac{1}{5}\int\cos(u)\,du
@wrong Match dx with du/5.
@why Substitution changes cos(5x+1) to cos(u), and dx=du/5 supplies the factor 1/5.
@step 40 | Choose a primitive of the scaled u-integrand.
@choice 42 | H(u)=-\frac{1}{5}\sin(u)
@choice 41 | H(u)=\frac{1}{5}\sin(u)
@choice 43 | H(u)=\frac{1}{5}\cos(u)
@answer 41
@feedback 42 | Differentiating negative sine gives negative cosine, whereas the converted coefficient is positive 1/5.
@feedback 43 | Differentiating cosine gives negative sine, not cosine. Use the sine primitive for cosine.
@after H(u)=\frac{1}{5}\sin(u)
@wrong Check the outer derivative rather than copying the outer function.
@why The u-derivative of sin(u)/5 is cos(u)/5, exactly the converted integrand.
@step 50 | Return a complete family in x.
@choice 52 | F(x)=\frac{1}{5}\sin(5x)+C
@choice 53 | F(x)=\frac{1}{5}\sin(5x+1)
@choice 51 | F(x)=\frac{1}{5}\sin(5x+1)+C
@answer 51
@feedback 52 | Back-substitute the whole u=5x+1. Dropping 1 changes the derivative to cos(5x), a different function.
@feedback 53 | This is one primitive only. Arbitrary real C is required to include every antiderivative.
@after F(x)=\frac{1}{5}\sin(5x+1)+C
@wrong Restore the complete argument and add C.
@why The same inner argument belongs inside the primitive's sine; the scalar stays 1/5.
@step 60 | Which derivative verifies the original function?
@choice 61 | F'(x)=\cos(5x+1)
@choice 62 | F'(x)=\frac{1}{5}\cos(5x+1)
@choice 63 | F'(x)=-\cos(5x+1)
@answer 61
@feedback 62 | Multiply by the inner derivative 5; (1/5) times 5 equals 1.
@feedback 63 | Sine differentiates to positive cosine, so no minus sign appears.
@after F'(x)=\cos(5x+1)
@wrong Check the sine derivative and its chain factor independently.
@why The coefficient is (1/5) times 5=1, giving exactly p on R. The constant-difference theorem on this connected interval makes the family complete.
@end

@question prod04_calc_substitution_q03 | A power integral
@template choices.v1
@version 1
@goal Find all antiderivatives of p in x and check the original derivative.
@given p(x)=(2-7x)^3
@domain Work on R. C is an arbitrary real constant; H is a primitive in u.
@read prod04_calc_substitution_r
@step 10 | Choose the complete base of the third power as u.
@choice 12 | u=7x-2
@choice 13 | u=x
@choice 11 | u=2-7x
@answer 11
@feedback 12 | This is the negative of the given base, so the original cube becomes -u cubed, not u cubed.
@feedback 13 | This identity substitution leaves the composite power unchanged. The requested base is 2-7x.
@after u=2-7x
@wrong Preserve the order and sign in the base.
@why The cube is the outer power and 2-7x is its complete inner expression.
@step 20 | Differentiate u to obtain the differential relation.
@choice 21 | du=-7\,dx
@choice 22 | du=7\,dx
@choice 23 | du=-5\,dx
@answer 21
@feedback 22 | The coefficient of x is -7, not +7; differentiating preserves that sign.
@feedback 23 | The constant 2 differentiates to zero, so the derivative is 0-7=-7, not 2-7.
@after du=-7\,dx
@wrong The constant contributes zero and the linear coefficient stays negative.
@why Since du=-7 dx, solving for the differential product gives dx=-du/7.
@step 30 | Convert the complete integral to u.
@choice 32 | \int p(x)\,dx=\frac{1}{7}\int u^3\,du
@choice 31 | \int p(x)\,dx=-\frac{1}{7}\int u^3\,du
@choice 33 | \int p(x)\,dx=-7\int u^3\,du
@answer 31
@feedback 32 | This loses the negative sign in dx=-du/7. The ratio of multiplier 1 to derivative -7 is -1/7.
@feedback 33 | Use the reciprocal -1/7, not the derivative -7 itself, when replacing dx.
@after \int p(x)\,dx=-\frac{1}{7}\int u^3\,du
@wrong Retain both the reciprocal and sign of the derivative coefficient.
@why The base becomes u, while dx contributes -1/7, so the complete scaled integrand is -u cubed/7.
@step 40 | Integrate that scaled power in u.
@choice 42 | H(u)=-\frac{1}{7}u^4
@choice 43 | H(u)=\frac{1}{28}u^4
@choice 41 | H(u)=-\frac{1}{28}u^4
@answer 41
@feedback 42 | Raising power 3 to 4 also requires division by 4. Otherwise the derivative coefficient is -4/7 instead of -1/7.
@feedback 43 | Division by positive 4 cannot remove the negative scale: (-1/7)/4=-1/28.
@after H(u)=-\frac{1}{28}u^4
@wrong Raise the exponent and divide the signed scalar by the new exponent.
@why The new coefficient is (-1/7)/4=-1/28. Its u-derivative is (-1/28) times 4 times u cubed=-u cubed/7.
@step 50 | Return the complete family to x.
@choice 51 | F(x)=-\frac{1}{28}(2-7x)^4+C
@choice 52 | H(u)=-\frac{1}{28}u^4+C
@choice 53 | F(x)=\frac{1}{28}(2-7x)^4+C
@answer 51
@feedback 52 | This is the correct constant family in the substituted coordinate, but the requested final family is in x. Replace u by 2-7x.
@feedback 53 | This flips the primitive's sign. Its derivative is negative (2-7x) cubed, not the original positive cube.
@after F(x)=-\frac{1}{28}(2-7x)^4+C
@wrong A correct u-family still needs back-substitution.
@why Insert the whole base into H and add arbitrary C; neither operation changes the coefficient -1/28.
@step 60 | Differentiate F and compare with the original p.
@choice 62 | F'(x)=-(2-7x)^3
@choice 61 | F'(x)=(2-7x)^3
@choice 63 | F'(x)=-\frac{1}{7}(2-7x)^3
@answer 61
@feedback 62 | The negative primitive coefficient and negative inner derivative multiply to a positive result.
@feedback 63 | This includes the outer derivative 4 but omits the inner derivative -7. Multiplying -1/7 by -7 gives 1.
@after F'(x)=(2-7x)^3
@wrong Multiply all three factors: primitive coefficient, new exponent and inner derivative.
@why The full coefficient is (-1/28) times 4 times (-7)=1. This proves F'=p on R, and the constant-difference theorem proves completeness of the family.
@end

@question prod04_calc_substitution_q04 | Another trigonometric integral
@template choices.v1
@version 1
@goal Find all antiderivatives of p in x and check the original derivative.
@given p(x)=\sin(8-3x)
@domain Work on R. C is an arbitrary real constant; H is a primitive in u.
@read prod04_calc_substitution_r
@step 10 | Choose the complete sine argument as u.
@choice 11 | u=8-3x
@choice 12 | u=3x-8
@choice 13 | u=-3x
@answer 11
@feedback 12 | This negates the argument; sine is odd, so the integrand would become -sin(u), not sin(u).
@feedback 13 | The constant 8 is inside the sine and must be included in the named argument.
@after u=8-3x
@wrong Copy the full argument with its original signs.
@why The outer function is sine, whose input is 8-3x.
@step 20 | Differentiate that substitution.
@choice 22 | du=3\,dx
@choice 21 | du=-3\,dx
@choice 23 | du=5\,dx
@answer 21
@feedback 22 | Differentiating -3x gives -3, not +3.
@feedback 23 | The derivative of 8 is zero, so compute 0-3 rather than 8-3.
@after du=-3\,dx
@wrong Differentiate before solving for dx.
@why The inner derivative is -3, hence dx=-du/3.
@step 30 | Convert the entire integral to u.
@choice 32 | \int p(x)\,dx=\frac{1}{3}\int\sin(u)\,du
@choice 33 | \int p(x)\,dx=-3\int\sin(u)\,du
@choice 31 | \int p(x)\,dx=-\frac{1}{3}\int\sin(u)\,du
@answer 31
@feedback 32 | The differential has a negative scale, so its factor is -1/3.
@feedback 33 | The derivative -3 must be inverted when replacing dx, giving -1/3 rather than -3.
@after \int p(x)\,dx=-\frac{1}{3}\int\sin(u)\,du
@wrong The converted scale comes from dx=-du/3.
@why The sine argument becomes u and the differential contributes -1/3; no other multiplier remains.
@step 40 | Find a primitive H in u.
@choice 41 | H(u)=\frac{1}{3}\cos(u)
@choice 42 | H(u)=-\frac{1}{3}\cos(u)
@choice 43 | H(u)=-\frac{1}{3}\sin(u)
@answer 41
@feedback 42 | The primitive of sin(u) is -cos(u); multiplying by -1/3 gives positive cos(u)/3, not negative.
@feedback 43 | The derivative of this expression is -cos(u)/3, while the converted integrand is -sin(u)/3.
@after H(u)=\frac{1}{3}\cos(u)
@wrong Separate the negative substitution scale from the negative sine primitive.
@why The two negatives multiply: (-1/3) times (-cos(u))=cos(u)/3. Its derivative is -sin(u)/3.
@step 50 | Return the complete family in x.
@choice 52 | F(x)=\frac{1}{3}\cos(8-3x)
@choice 51 | F(x)=\frac{1}{3}\cos(8-3x)+C
@choice 53 | F(x)=-\frac{1}{3}\cos(8-3x)+C
@answer 51
@feedback 52 | A single primitive excludes every nonzero constant shift; include arbitrary C for all antiderivatives.
@feedback 53 | This extra minus sign would give derivative -sin(8-3x). Back-substitution does not change H's sign.
@after F(x)=\frac{1}{3}\cos(8-3x)+C
@wrong Preserve the primitive's positive scalar and include arbitrary C.
@why Replace u by 8-3x in the positive cosine primitive and allow any real constant.
@step 60 | Which derivative checks the original sine integrand?
@choice 62 | F'(x)=-\sin(8-3x)
@choice 63 | F'(x)=-\frac{1}{3}\sin(8-3x)
@choice 61 | F'(x)=\sin(8-3x)
@answer 61
@feedback 62 | The cosine derivative contributes -1 and the inner derivative contributes -3; their product is positive 3.
@feedback 63 | This omits the chain factor -3, which changes coefficient -1/3 into +1.
@after F'(x)=\sin(8-3x)
@wrong Multiply (1/3), the cosine derivative's minus sign and the inner derivative -3.
@why The product (1/3) times (-1) times (-3)=1 gives exactly p on R. Any other primitive differs by a constant on this connected domain.
@end

@question prod04_calc_substitution_q05 | An integral with a variable multiplier
@template choices.v1
@version 1
@goal Find all antiderivatives of p in x and check the original derivative.
@given p(x)=xe^{x^2}
@domain Work on R. C is an arbitrary real constant; H is a primitive in u.
@read prod04_calc_substitution_r
@step 10 | Choose the complete exponential argument as u.
@choice 12 | u=x
@choice 11 | u=x^2
@choice 13 | u=2x
@answer 11
@feedback 12 | This identity substitution leaves the exponent as u squared, so it does not replace the whole argument by u.
@feedback 13 | The derivative 2x is not the argument. The exponential input is x squared.
@after u=x^2
@wrong Name the inner function before differentiating it.
@why Choosing x squared makes the exponential e to the u and exposes a derivative proportional to the multiplier x.
@step 20 | Compute du for the chosen substitution.
@choice 22 | du=x\,dx
@choice 23 | du=2\,dx
@choice 21 | du=2x\,dx
@answer 21
@feedback 22 | The derivative of x squared is 2x; the exponent contributes the missing coefficient 2.
@feedback 23 | Differentiation reduces the exponent from 2 to 1, not to 0, so a factor x remains.
@after du=2x\,dx
@wrong Apply the power rule including its coefficient and remaining power.
@why The differential product is du=2x dx, so x dx=du/2. This identity does not divide by x and remains valid at zero.
@step 30 | Convert the whole integral, including the multiplier x.
@choice 31 | \int p(x)\,dx=\frac{1}{2}\int e^u\,du
@choice 32 | \int p(x)\,dx=\int e^u\,du
@choice 33 | \int p(x)\,dx=2\int e^u\,du
@answer 31
@feedback 32 | The original multiplier is x, half of g'=2x; the complete differential product contributes 1/2.
@feedback 33 | Since x dx=du/2, the scale is 1/2 rather than 2.
@after \int p(x)\,dx=\frac{1}{2}\int e^u\,du
@wrong Replace the whole x dx pair, not dx alone.
@why The pair x dx becomes du/2 and the argument x squared becomes u, leaving no x in the converted integral.
@step 40 | Integrate the scaled u-integrand.
@choice 42 | H(u)=e^u
@choice 41 | H(u)=\frac{1}{2}e^u
@choice 43 | H(u)=-\frac{1}{2}e^u
@answer 41
@feedback 42 | Its derivative is twice the converted integrand. The factor 1/2 must stay with the exponential.
@feedback 43 | Exponential integration introduces no minus sign; the derivative here has the wrong sign.
@after H(u)=\frac{1}{2}e^u
@wrong Retain the scalar while integrating the exponential.
@why The exponential is unchanged by integration, and its coefficient remains 1/2.
@step 50 | Return the complete antiderivative family in x.
@choice 52 | H(u)=\frac{1}{2}e^u+C
@choice 53 | F(x)=\frac{1}{2}e^{x^2}
@choice 51 | F(x)=\frac{1}{2}e^{x^2}+C
@answer 51
@feedback 52 | This is a correct family in u, but the requested final coordinate is x. Replace u by x squared.
@feedback 53 | This gives only one constant choice. The full family includes every real C.
@after F(x)=\frac{1}{2}e^{x^2}+C
@wrong Finish both operations: back-substitution and the arbitrary constant.
@why Inserting x squared into H gives the x-primitive; adding arbitrary C represents all constant shifts.
@step 60 | Differentiate the returned family against p.
@choice 61 | F'(x)=xe^{x^2}
@choice 62 | F'(x)=\frac{1}{2}e^{x^2}
@choice 63 | F'(x)=2xe^{x^2}
@answer 61
@feedback 62 | The inner derivative is 2x, which must multiply the outer derivative.
@feedback 63 | The primitive's 1/2 cancels the chain factor's 2, leaving x rather than 2x.
@after F'(x)=xe^{x^2}
@wrong Multiply the complete chain factor 2x by 1/2.
@why The derivative is (1/2) times e to the x squared times 2x=x e to the x squared, exactly p everywhere on R. Connectedness makes the constant family complete.
@end

@question prod04_calc_substitution_q06 | A polynomial composition
@template choices.v1
@version 1
@goal Find all antiderivatives of p in x and check the original derivative.
@given p(x)=x(4x^2+7)^2
@domain Work on R. C is an arbitrary real constant; H is a primitive in u.
@read prod04_calc_substitution_r
@step 10 | Choose the complete base of the squared expression as u.
@choice 12 | u=x^2
@choice 13 | u=8x
@choice 11 | u=4x^2+7
@answer 11
@feedback 12 | This useful partial substitution leaves (4u+7) squared; the requested complete base is 4x squared+7.
@feedback 13 | The expression 8x is the inner derivative, not the base that is squared.
@after u=4x^2+7
@wrong Distinguish the complete inner function from its derivative.
@why The outer operation squares its input, and that input is 4x squared+7.
@step 20 | Differentiate the selected u.
@choice 21 | du=8x\,dx
@choice 22 | du=4x\,dx
@choice 23 | du=(8x+7)\,dx
@answer 21
@feedback 22 | Differentiating 4x squared multiplies 4 by 2, producing 8x rather than 4x.
@feedback 23 | The constant 7 differentiates to zero and does not remain in du.
@after du=8x\,dx
@wrong Apply the power rule and remove only the derivative of the constant.
@why The derivative is 8x+0, so x dx=du/8 as a differential-product identity, including at x=0.
@step 30 | Convert the complete integral to u.
@choice 32 | \int p(x)\,dx=\frac{1}{4}\int u^2\,du
@choice 31 | \int p(x)\,dx=\frac{1}{8}\int u^2\,du
@choice 33 | \int p(x)\,dx=8\int u^2\,du
@answer 31
@feedback 32 | The derivative coefficient is 8, not 4; the original x dx is one eighth of du.
@feedback 33 | Multiplying by 8 reverses the differential relation. Use x dx=du/8.
@after \int p(x)\,dx=\frac{1}{8}\int u^2\,du
@wrong Replace the full multiplier-differential pair.
@why The squared base becomes u squared and x dx contributes 1/8, leaving a pure u-integral.
@step 40 | Find a primitive H of the scaled u-integrand.
@choice 42 | H(u)=\frac{1}{8}u^3
@choice 43 | H(u)=\frac{1}{16}u^3
@choice 41 | H(u)=\frac{1}{24}u^3
@answer 41
@feedback 42 | Raising the exponent to 3 requires division by 3; otherwise the derivative coefficient is 3/8 rather than 1/8.
@feedback 43 | Divide by the new exponent 3, not the old exponent 2: (1/8)/3=1/24.
@after H(u)=\frac{1}{24}u^3
@wrong Divide the scale by the new power.
@why The primitive of u squared is u cubed/3, so the complete scalar is 1/(8 times 3)=1/24.
@step 50 | Return the complete family in x.
@choice 51 | F(x)=\frac{1}{24}(4x^2+7)^3+C
@choice 52 | F(x)=\frac{1}{24}(4x^2)^3+C
@choice 53 | F(x)=\frac{1}{8}(4x^2+7)^3+C
@answer 51
@feedback 52 | Omitting the +7 changes the inner function. A constant outside the cube cannot repair that missing term inside it.
@feedback 53 | Back-substitution does not undo division by the new exponent 3; this derivative would be three times p.
@after F(x)=\frac{1}{24}(4x^2+7)^3+C
@wrong Restore the entire base and retain coefficient 1/24.
@why Replace u by 4x squared+7 in H. Arbitrary C then includes every constant shift.
@step 60 | Differentiate F to verify the original integrand.
@choice 62 | F'(x)=\frac{1}{8}(4x^2+7)^2
@choice 61 | F'(x)=x(4x^2+7)^2
@choice 63 | F'(x)=3x(4x^2+7)^2
@answer 61
@feedback 62 | This differentiates the outer cube but omits the inner derivative 8x.
@feedback 63 | The coefficient is (1/24) times 3 times 8=1, not 3.
@after F'(x)=x(4x^2+7)^2
@wrong Include both the cube's derivative and the inner derivative.
@why Multiplying (1/24) times 3 times 8x gives x, so the derivative equals p identically on R. Every other primitive differs by a constant on this connected interval.
@end

@question prod04_calc_substitution_q07 | An integral on a stated interval
@template choices.v1
@version 1
@goal Find all antiderivatives of p in x and check the original derivative.
@given p(x)=\frac{x^2}{5x^3+1}
@domain Work on x>0, a connected interval. C is an arbitrary real constant; H is a primitive in u. Preserve this original domain.
@read prod04_calc_substitution_r
@step 10 | Choose the complete reciprocal denominator as u.
@choice 11 | u=5x^3+1
@choice 12 | u=x^2
@choice 13 | u=5x^3
@answer 11
@feedback 12 | The numerator is not the reciprocal's argument. The denominator 5x cubed+1 has derivative proportional to x squared.
@feedback 13 | Omitting 1 leaves the denominator as u+1 rather than u.
@after u=5x^3+1
@wrong Use the whole denominator and retain the stated domain.
@why The reciprocal outer function has input 5x cubed+1. For x>0 that input exceeds 1, so it never vanishes.
@step 20 | Differentiate u.
@choice 22 | du=5x^2\,dx
@choice 21 | du=15x^2\,dx
@choice 23 | du=(15x^2+1)\,dx
@answer 21
@feedback 22 | The cube contributes factor 3, so 5 times 3=15 on x squared.
@feedback 23 | The derivative of the constant 1 is zero, not 1.
@after du=15x^2\,dx
@wrong Include the power factor and differentiate the constant correctly.
@why The derivative is 15x squared, hence x squared dx=du/15.
@step 30 | Convert every factor to u.
@choice 32 | \int p(x)\,dx=\frac{1}{5}\int\frac{1}{u}\,du
@choice 33 | \int p(x)\,dx=15\int\frac{1}{u}\,du
@choice 31 | \int p(x)\,dx=\frac{1}{15}\int\frac{1}{u}\,du
@answer 31
@feedback 32 | The complete inner derivative coefficient is 15, so the scale is 1/15 rather than 1/5.
@feedback 33 | The original numerator-differential pair is du/15, not 15 du.
@after \int p(x)\,dx=\frac{1}{15}\int\frac{1}{u}\,du
@wrong Match x squared dx to du/15 while replacing the denominator by u.
@why Both substitutions together give (1/15) times the integral of 1/u. Here u>1, so the reciprocal and its log primitive are defined.
@step 40 | Choose a primitive H of the reciprocal u-integrand.
@choice 41 | H(u)=\frac{1}{15}\ln|u|
@choice 42 | H(u)=\frac{1}{15}u^0
@choice 43 | H(u)=15\ln|u|
@answer 41
@feedback 42 | u to power zero is constant and has derivative zero. The power formula would divide by zero for exponent -1; use the logarithmic rule.
@feedback 43 | Its derivative is 15/u, while the converted integrand is 1/(15u). Preserve the coefficient 1/15.
@after H(u)=\frac{1}{15}\ln|u|
@wrong A reciprocal requires the logarithmic primitive on a nonzero domain.
@why Differentiating ln|u| gives 1/u for u nonzero. Multiplying by 1/15 gives the entire converted integrand.
@step 50 | Return the complete family in x on the original interval.
@choice 52 | F(x)=\frac{1}{15}\ln|5x^3+1|
@choice 51 | F(x)=\frac{1}{15}\ln|5x^3+1|+C
@choice 53 | F(x)=\frac{1}{15}\ln|5x^3|+C
@answer 51
@feedback 52 | This omits all nonzero constant shifts. One arbitrary real C is needed on the connected interval x>0.
@feedback 53 | Dropping 1 changes the derivative to 1/(5x), which is not x squared/(5x cubed+1).
@after F(x)=\frac{1}{15}\ln|5x^3+1|+C
@wrong Restore the entire denominator inside the logarithm.
@why The argument is nonzero throughout x>0. Substitute 5x cubed+1 into H and add arbitrary C without extending the original domain.
@step 60 | Which derivative checks the original p on x>0?
@choice 62 | F'(x)=\frac{1}{15(5x^3+1)}
@choice 63 | F'(x)=\frac{15x^2}{5x^3+1}
@choice 61 | F'(x)=\frac{x^2}{5x^3+1}
@answer 61
@feedback 62 | This omits the inner derivative 15x squared from the logarithmic chain rule.
@feedback 63 | The primitive coefficient 1/15 cancels the 15 in the inner derivative, leaving x squared in the numerator.
@after F'(x)=\frac{x^2}{5x^3+1}
@wrong Multiply (1/15), the reciprocal argument and the inner derivative.
@why The derivative is (1/15) times 15x squared divided by (5x cubed+1), exactly p. The denominator stays positive, and connectedness makes the single-C family complete on x>0.
@end

@question prod04_calc_substitution_q08 | A second restricted interval
@template choices.v1
@version 1
@goal Find all antiderivatives of p in x and check the original derivative.
@given p(x)=\frac{1}{11x-9}
@domain Work on x<0, a connected interval. C is an arbitrary real constant; H is a primitive in u. Preserve this original domain.
@read prod04_calc_substitution_r
@step 10 | Choose the complete reciprocal denominator as u.
@choice 12 | u=11x
@choice 11 | u=11x-9
@choice 13 | u=9-11x
@answer 11
@feedback 12 | Omitting -9 leaves a denominator u-9 rather than u.
@feedback 13 | This is the negative of the given denominator; its reciprocal would require an additional minus sign. It is not the requested complete denominator itself.
@after u=11x-9
@wrong Preserve the whole signed denominator.
@why The reciprocal input is 11x-9. Since x<0, this is less than -9 and never zero, so the original integrand is defined throughout the connected interval.
@step 20 | Differentiate the chosen substitution.
@choice 22 | du=2\,dx
@choice 23 | du=-11\,dx
@choice 21 | du=11\,dx
@answer 21
@feedback 22 | The derivative of -9 is zero, so compute 11+0, not 11-9.
@feedback 23 | A negative value of u does not imply a negative derivative. Its x coefficient is positive 11.
@after du=11\,dx
@wrong Distinguish the sign of the function from the sign of its derivative.
@why Differentiating 11x-9 gives 11, hence dx=du/11 even though u is negative on this domain.
@step 30 | Convert the complete integral to u.
@choice 31 | \int p(x)\,dx=\frac{1}{11}\int\frac{1}{u}\,du
@choice 32 | \int p(x)\,dx=-\frac{1}{11}\int\frac{1}{u}\,du
@choice 33 | \int p(x)\,dx=11\int\frac{1}{u}\,du
@answer 31
@feedback 32 | The negative sign of 1/u is already in that function's value; dx contributes positive 1/11, not another minus sign.
@feedback 33 | Since du=11 dx, replace dx by du/11, not 11 du.
@after \int p(x)\,dx=\frac{1}{11}\int\frac{1}{u}\,du
@wrong Let 1/u retain its sign and use the positive differential scale.
@why The ratio of original multiplier 1 to inner derivative 11 is 1/11, with u<-9 throughout the domain.
@step 40 | Choose a real primitive H on this negative u-interval.
@choice 42 | H(u)=\frac{1}{11}\ln(u)
@choice 41 | H(u)=\frac{1}{11}\ln|u|
@choice 43 | H(u)=-\frac{1}{11}\ln|u|
@answer 41
@feedback 42 | Here u<-9, so ln(u) is not real. Absolute value makes the logarithm's argument positive without admitting u=0.
@feedback 43 | Differentiating ln|u| already gives the negative number 1/u on this interval. The added minus sign reverses the required derivative.
@after H(u)=\frac{1}{11}\ln|u|
@wrong Use a real logarithm on the stated negative interval.
@why For negative u, ln|u|=ln(-u), whose derivative is (-1)/(-u)=1/u. Thus H'=1/(11u) with the correct sign.
@step 50 | Return the complete real family on x<0.
@choice 52 | F(x)=\frac{1}{11}\ln(11x-9)+C
@choice 53 | F(x)=\frac{1}{11}\ln|11x-9|
@choice 51 | F(x)=\frac{1}{11}\ln|11x-9|+C
@answer 51
@feedback 52 | The argument 11x-9 is negative for every allowed x, so this is not a real-valued family on the original domain.
@feedback 53 | A single primitive is not the full family. Any real constant shift has the same derivative on x<0.
@after F(x)=\frac{1}{11}\ln|11x-9|+C
@wrong Keep the absolute value and include one arbitrary constant on the connected interval.
@why Back-substitution preserves the nonzero argument. The absolute value allows its negative sign, and C remains arbitrary because no function value was prescribed.
@step 60 | Differentiate the real family against the original p.
@choice 61 | F'(x)=\frac{1}{11x-9}
@choice 62 | F'(x)=-\frac{1}{11x-9}
@choice 63 | F'(x)=\frac{11}{11x-9}
@answer 61
@feedback 62 | The absolute-value logarithm has derivative g'/g, not -g'/g. The sign is already carried by the negative denominator.
@feedback 63 | The primitive coefficient 1/11 cancels the inner derivative 11, leaving numerator 1.
@after F'(x)=\frac{1}{11x-9}
@wrong Differentiate ln|g| as g'/g while preserving the original interval.
@why The product (1/11) times 11/(11x-9) equals p. The pole x=9/11 is outside x<0, and connectedness proves completeness with one arbitrary C.
@end

@question prod04_calc_substitution_q09 | A bounded rational integral
@template choices.v1
@version 1
@goal Calculate I exactly by substitution and check the original derivative and endpoint order.
@given p(x)=\frac{x}{1+4x^2},\quad I=\int_{1}^{2}p(x)\,dx
@domain p is continuous on the closed interval between the displayed limits. Keep their order. H is a primitive in u; P(x)=H(g(x)).
@read prod04_calc_substitution_r
@step 10 | Select a substitution that replaces the complete denominator and exposes its derivative pair.
@choice 12 | u=x
@choice 13 | u=4x^2
@choice 11 | u=1+4x^2
@answer 11
@feedback 12 | The identity substitution is legal but does not expose the reciprocal pair as 1/u with a constant differential scale.
@feedback 13 | This leaves the denominator as 1+u. The requested complete-denominator substitution includes the constant 1.
@after u=1+4x^2
@wrong Choose a whole inner function whose derivative matches the numerator up to a constant.
@why The denominator's derivative is proportional to x. Naming the full denominator u makes the outer function a reciprocal; it stays positive on [1,2].
@step 20 | Compute du for this method.
@choice 21 | du=8x\,dx
@choice 22 | du=4x\,dx
@choice 23 | du=(1+8x)\,dx
@answer 21
@feedback 22 | The squared power contributes a factor 2, so the derivative coefficient is 4 times 2=8.
@feedback 23 | The constant 1 has derivative zero and must not remain.
@after du=8x\,dx
@wrong Differentiate the whole denominator term by term.
@why The derivative is 8x, so the complete numerator-differential pair x dx equals du/8.
@step 30 | Map the original lower and upper limits to an ordered pair of u-bounds.
@choice 32 | (g(a),g(b))=(17,5)
@choice 31 | (g(a),g(b))=(5,17)
@choice 33 | (g(a),g(b))=(1,2)
@answer 31
@feedback 32 | The lower x-limit 1 maps to 1+4=5; the upper x-limit 2 maps to 1+16=17. This option swaps their positions.
@feedback 33 | These are the x-bounds, not their images under g. Compute 1+4x squared at each endpoint.
@after (g(a),g(b))=(5,17)
@wrong Substitute each original endpoint separately without sorting or copying.
@why With a=1 and b=2, g(a)=1+4(1 squared)=5 and g(b)=1+4(2 squared)=17.
@step 40 | Convert the entire bounded integral to the u-coordinate.
@choice 42 | I=\frac{1}{8}\int_{1}^{2}\frac{1}{u}\,du
@choice 43 | I=8\int_{5}^{17}\frac{1}{u}\,du
@choice 41 | I=\frac{1}{8}\int_{5}^{17}\frac{1}{u}\,du
@answer 41
@feedback 42 | This mixes the u-integrand with the original x-bounds. The correct images are 5 and 17.
@feedback 43 | The bounds are right, but x dx=du/8 gives factor 1/8, not 8.
@after I=\frac{1}{8}\int_{5}^{17}\frac{1}{u}\,du
@wrong Change the multiplier, denominator, differential and bounds together.
@why The pair x dx becomes du/8, the denominator becomes u, and the bounds become 5 then 17. Since this interval stays positive, no pole is crossed.
@step 50 | Choose H for the fully scaled u-integrand.
@choice 51 | H(u)=\frac{1}{8}\ln|u|
@choice 52 | H(u)=8\ln|u|
@choice 53 | H(u)=\frac{1}{8}u^0
@answer 51
@feedback 52 | Its derivative is 8/u rather than 1/(8u). Keep the converted coefficient 1/8.
@feedback 53 | A power-zero expression is constant and differentiates to zero, not to a reciprocal.
@after H(u)=\frac{1}{8}\ln|u|
@wrong Integrate the reciprocal with the logarithm and preserve its scalar.
@why The derivative of H is 1/(8u) on the positive interval [5,17]. Any added constant would cancel in endpoint subtraction.
@step 60 | Evaluate H at the upper u-bound minus the lower u-bound.
@choice 62 | I=\frac{1}{8}(\ln(5)-\ln(17))
@choice 61 | I=\frac{1}{8}(\ln(17)-\ln(5))
@choice 63 | I=\frac{1}{8}\ln(12)
@answer 61
@feedback 62 | This computes H(5)-H(17), reversing the required order and sign.
@feedback 63 | A difference of logarithms is not the logarithm of a difference. It is ln(17/5), not ln(17-5).
@after I=\frac{1}{8}(\ln(17)-\ln(5))
@wrong Subtract primitive values in displayed-limit order.
@why H(17)=ln(17)/8 and H(5)=ln(5)/8. Their upper-minus-lower difference is positive, matching x/(1+4x squared)>0 on [1,2].
@step 70 | Check P(x)=H(g(x)) against the original p and the original endpoint difference P(b)-P(a).
@choice 72 | P'(x)=\frac{1}{1+4x^2},\quad I=\frac{1}{8}(\ln(17)-\ln(5))
@choice 73 | P'(x)=\frac{x}{1+4x^2},\quad I=\frac{1}{8}(\ln(5)-\ln(17))
@choice 71 | P'(x)=\frac{x}{1+4x^2},\quad I=\frac{1}{8}(\ln(17)-\ln(5))
@answer 71
@feedback 72 | Differentiating ln|1+4x squared|/8 contributes inner derivative 8x; the numerator is x, not 1.
@feedback 73 | The derivative is correct, but P(2)-P(1) is ln(17)/8 minus ln(5)/8, not the reverse.
@after P'(x)=\frac{x}{1+4x^2},\quad I=\frac{1}{8}(\ln(17)-\ln(5))
@wrong Check both the derivative identity and the original x-endpoint order.
@why P'= (1/8) times 8x/(1+4x squared)=p. Also P(2)=ln(17)/8 and P(1)=ln(5)/8. Continuity on [1,2] justifies their difference by the Fundamental Theorem of Calculus.
@end

@question prod04_calc_substitution_q10 | A bounded polynomial integral
@template choices.v1
@version 1
@goal Calculate I exactly by substitution and check the original derivative and endpoint order.
@given p(x)=(3-2x)^2,\quad I=\int_{0}^{1}p(x)\,dx
@domain p is continuous on the closed interval between the displayed limits. Keep their order. H is a primitive in u; P(x)=H(g(x)).
@read prod04_calc_substitution_r
@step 10 | Select the complete squared base as u to obtain a pure power integral.
@choice 11 | u=3-2x
@choice 12 | u=x
@choice 13 | u=2x-3
@answer 11
@feedback 12 | This identity substitution is legal but leaves (3-2u) squared, so it does not meet the requested pure-power conversion.
@feedback 13 | This negated base can also support a valid method because the square is even, but it is not the specified base 3-2x. Its differential and bounds would need a different route.
@after u=3-2x
@wrong Use the precise base requested, then carry that method consistently.
@why The full inner base 3-2x becomes u, so the outer factor becomes u squared. Its decreasing behavior will matter when transforming bounds.
@step 20 | Differentiate the selected substitution.
@choice 22 | du=2\,dx
@choice 21 | du=-2\,dx
@choice 23 | du=1\,dx
@answer 21
@feedback 22 | The x coefficient in 3-2x is -2, so du has a negative factor.
@feedback 23 | The derivative of 3 is zero: compute 0-2, not 3-2.
@after du=-2\,dx
@wrong The constant vanishes and the linear coefficient retains its sign.
@why du=-2 dx gives dx=-du/2. This negative scale must remain unless the u-limits are explicitly reversed as well.
@step 30 | Map the lower and upper x-limits in their original order.
@choice 32 | (g(a),g(b))=(1,3)
@choice 33 | (g(a),g(b))=(0,1)
@choice 31 | (g(a),g(b))=(3,1)
@answer 31
@feedback 32 | Sorting the images changes their order. The lower displayed x-limit 0 maps to 3 and the upper x-limit 1 maps to 1.
@feedback 33 | These are untransformed x-bounds. Apply g(x)=3-2x to each one.
@after (g(a),g(b))=(3,1)
@wrong Preserve displayed-limit order even when the substitution is decreasing.
@why g(0)=3-0=3 and g(1)=3-2=1. The u-integral therefore runs from 3 down to 1.
@step 40 | Convert the whole integral using those ordered u-bounds.
@choice 41 | I=-\frac{1}{2}\int_{3}^{1}u^2\,du
@choice 42 | I=\frac{1}{2}\int_{3}^{1}u^2\,du
@choice 43 | I=-\frac{1}{2}\int_{0}^{1}u^2\,du
@answer 41
@feedback 42 | The ordered bounds are right, but dx=-du/2 contributes a negative scale. Removing it would negate the original result.
@feedback 43 | These are the original x-bounds, not 3 and 1. A u-integrand requires transformed u-bounds.
@after I=-\frac{1}{2}\int_{3}^{1}u^2\,du
@wrong Retain both the negative scale and the reversed numerical order of the images.
@why Replacing the base, differential and endpoints gives -1/2 times the integral from 3 to 1 of u squared. The negative scale and downward limits compensate.
@step 50 | Choose H for the fully scaled u-integrand.
@choice 52 | H(u)=-\frac{1}{2}u^3
@choice 51 | H(u)=-\frac{1}{6}u^3
@choice 53 | H(u)=\frac{1}{6}u^3
@answer 51
@feedback 52 | Division by the new exponent 3 is missing, so its derivative is -3u squared/2 rather than -u squared/2.
@feedback 53 | The negative scale remains when dividing by positive 3; the coefficient is -1/6.
@after H(u)=-\frac{1}{6}u^3
@wrong Divide the signed scale -1/2 by 3.
@why The primitive coefficient is (-1/2)/3=-1/6, whose derivative is -u squared/2.
@step 60 | Evaluate upper displayed u-endpoint minus lower displayed u-endpoint.
@choice 62 | I=-\frac{13}{3}
@choice 63 | I=\frac{14}{3}
@choice 61 | I=\frac{13}{3}
@answer 61
@feedback 62 | This reverses H(1)-H(3). The correct subtraction is -1/6-(-27/6)=26/6=13/3.
@feedback 63 | Adding magnitudes 1/6+27/6 gives 28/6, but the signed subtraction is -1/6+27/6=26/6.
@after I=\frac{13}{3}
@wrong Substitute into the negative primitive before subtracting its signed values.
@why H(1)=-1/6 and H(3)=-27/6. Thus I=(-1+27)/6=26/6=13/3, positive as the original squared integrand requires.
@step 70 | Check P(x)=H(g(x)) and the original endpoint difference P(1)-P(0).
@choice 71 | P'(x)=(3-2x)^2,\quad I=\frac{13}{3}
@choice 72 | P'(x)=-(3-2x)^2,\quad I=\frac{13}{3}
@choice 73 | P'(x)=(3-2x)^2,\quad I=-\frac{13}{3}
@answer 71
@feedback 72 | The chain coefficient is (-1/6) times 3 times (-2)=+1, not -1.
@feedback 73 | The derivative is right, but P(1)-P(0)=-1/6-(-27/6)=+13/3.
@after P'(x)=(3-2x)^2,\quad I=\frac{13}{3}
@wrong Verify both the chain coefficient and the subtraction using x=1 and x=0.
@why P(x)=-(3-2x) cubed/6 differentiates to p. Its endpoint values -1/6 and -27/6 give 13/3. The polynomial is continuous on [0,1], so the original integral equals this difference.
@end

@question prod04_calc_substitution_q11 | A bounded exponential integral
@template choices.v1
@version 1
@goal Calculate I exactly by substitution and check the original derivative and endpoint order.
@given p(x)=xe^{x^2},\quad I=\int_{0}^{1}p(x)\,dx
@domain p is continuous on the closed interval between the displayed limits. Keep their order. H is a primitive in u; P(x)=H(g(x)).
@read prod04_calc_substitution_r
@step 10 | Select a substitution using the complete exponent to expose a function-derivative pair.
@choice 12 | u=x
@choice 11 | u=x^2
@choice 13 | u=2x
@answer 11
@feedback 12 | This legal identity substitution leaves u times e to the u squared; it does not expose the requested pure exponential integral.
@feedback 13 | The expression 2x is the derivative of the exponent, not the exponent itself.
@after u=x^2
@wrong Match the exponential argument with the original multiplier.
@why With u=x squared, the derivative 2x is twice the multiplier x, giving a constant scale and a pure exponential in u.
@step 20 | Differentiate the chosen substitution.
@choice 22 | du=x\,dx
@choice 23 | du=2\,dx
@choice 21 | du=2x\,dx
@answer 21
@feedback 22 | Differentiating the squared power supplies a factor 2.
@feedback 23 | The power drops from 2 to 1, leaving x as well as coefficient 2.
@after du=2x\,dx
@wrong Apply the full power rule.
@why du=2x dx gives x dx=du/2. This product identity remains valid at the lower endpoint zero without dividing by x.
@step 30 | Map the original lower and upper endpoints to u.
@choice 31 | (g(a),g(b))=(0,1)
@choice 32 | (g(a),g(b))=(1,0)
@choice 33 | (g(a),g(b))=(0,2)
@answer 31
@feedback 32 | The original lower endpoint 0 maps to 0 squared=0; the upper endpoint 1 maps to 1 squared=1. This swaps them.
@feedback 33 | Evaluate g, not g': g(1)=1 squared=1, whereas 2 is the derivative value at 1.
@after (g(a),g(b))=(0,1)
@wrong Even when the numerical bounds stay the same, verify both images.
@why Here g(0)=0 and g(1)=1. The unchanged numbers result from substitution, not from a rule allowing x-bounds to be copied blindly.
@step 40 | Convert the entire definite integral to u.
@choice 42 | I=\int_{0}^{1}e^u\,du
@choice 41 | I=\frac{1}{2}\int_{0}^{1}e^u\,du
@choice 43 | I=\frac{1}{2}\int_{0}^{2}e^u\,du
@answer 41
@feedback 42 | The multiplier x is only half of g'=2x, so the scale 1/2 is required.
@feedback 43 | The upper u-bound is g(1)=1, not the derivative value 2.
@after I=\frac{1}{2}\int_{0}^{1}e^u\,du
@wrong Use the differential scale and the independently computed endpoint images.
@why The original x dx becomes du/2, the exponential argument becomes u, and the ordered bounds are 0 then 1.
@step 50 | Choose a primitive H of the scaled u-integrand.
@choice 52 | H(u)=e^u
@choice 53 | H(u)=-\frac{1}{2}e^u
@choice 51 | H(u)=\frac{1}{2}e^u
@answer 51
@feedback 52 | This primitive differentiates to twice the converted integrand; retain coefficient 1/2.
@feedback 53 | There is no negative derivative or substitution factor here, so the coefficient stays positive.
@after H(u)=\frac{1}{2}e^u
@wrong Preserve the converted scalar under exponential integration.
@why H'(u)=e to the u divided by 2, exactly the scaled u-integrand.
@step 60 | Evaluate H(1)-H(0) exactly.
@choice 61 | I=\frac{1}{2}(e^1-1)
@choice 62 | I=\frac{1}{2}e^1
@choice 63 | I=\frac{1}{2}(1-e^1)
@answer 61
@feedback 62 | The lower endpoint contributes H(0)=e to power zero divided by 2=1/2, not zero.
@feedback 63 | This subtracts upper from lower; retain H(1)-H(0), which is positive.
@after I=\frac{1}{2}(e^1-1)
@wrong Remember that e to power zero is 1 and preserve endpoint order.
@why H(1)=e/2 and H(0)=1/2, so their difference is (e-1)/2. Since the original integrand is nonnegative and positive for x>0, the result is positive.
@step 70 | Check P(x)=H(g(x)) against the original integrand and endpoints.
@choice 72 | P'(x)=\frac{1}{2}e^{x^2},\quad I=\frac{1}{2}(e^1-1)
@choice 71 | P'(x)=xe^{x^2},\quad I=\frac{1}{2}(e^1-1)
@choice 73 | P'(x)=xe^{x^2},\quad I=\frac{1}{2}e^1
@answer 71
@feedback 72 | The chain factor 2x is missing; it changes coefficient 1/2 into x in the original derivative.
@feedback 73 | The derivative is right, but P(0)=1/2 must be subtracted from P(1)=e/2.
@after P'(x)=xe^{x^2},\quad I=\frac{1}{2}(e^1-1)
@wrong Verify the chain identity and both original endpoint values, including zero.
@why P=e to the x squared divided by 2 has derivative (1/2)e to the x squared times 2x=p. Its values at 1 and 0 are e/2 and 1/2. Continuity on [0,1] proves the stated definite value.
@end

@question prod04_calc_substitution_q12 | A bounded trigonometric integral
@template choices.v1
@version 1
@goal Calculate I exactly by substitution and check the original derivative and endpoint order.
@given p(x)=x\cos(x^2),\quad I=\int_{0}^{1}p(x)\,dx
@domain p is continuous on the closed interval between the displayed limits. Keep their order. H is a primitive in u; P(x)=H(g(x)).
@read prod04_calc_substitution_r
@step 10 | Select the complete cosine argument as u to expose a constant-scaled derivative pair.
@choice 12 | u=x
@choice 13 | u=2x
@choice 11 | u=x^2
@answer 11
@feedback 12 | An identity substitution is legal, but leaves u cos(u squared) rather than the requested pure cosine integral.
@feedback 13 | This is the inner derivative, not the cosine's argument. The requested inner function is x squared.
@after u=x^2
@wrong Choose the cosine argument and then compare its derivative with x.
@why The argument x squared has derivative 2x, a constant multiple of the original multiplier. This method converts the entire integral to a scaled cosine in u.
@step 20 | Differentiate the selected u.
@choice 21 | du=2x\,dx
@choice 22 | du=x\,dx
@choice 23 | du=2\,dx
@answer 21
@feedback 22 | The squared power contributes coefficient 2 to its derivative.
@feedback 23 | The derivative retains x to power 1; it is not just the coefficient 2.
@after du=2x\,dx
@wrong Keep both the derivative coefficient and remaining power.
@why du=2x dx yields x dx=du/2 as a product identity that is valid even at zero.
@step 30 | Map the lower and upper original limits to an ordered u-pair.
@choice 32 | (g(a),g(b))=(1,0)
@choice 31 | (g(a),g(b))=(0,1)
@choice 33 | (g(a),g(b))=(0,2)
@answer 31
@feedback 32 | The original lower limit 0 maps to 0, and the upper limit 1 maps to 1; this reverses their order.
@feedback 33 | The upper image is 1 squared=1, not the derivative value 2.
@after (g(a),g(b))=(0,1)
@wrong Evaluate g at both limits even if their numerical values happen to stay unchanged.
@why g(0)=0 squared=0 and g(1)=1 squared=1; these are the correct u-bounds in their original order.
@step 40 | Convert all factors and bounds to u.
@choice 42 | I=\int_{0}^{1}\cos(u)\,du
@choice 43 | I=\frac{1}{2}\int_{0}^{2}\cos(u)\,du
@choice 41 | I=\frac{1}{2}\int_{0}^{1}\cos(u)\,du
@answer 41
@feedback 42 | The original multiplier-differential pair is du/2, so omitting 1/2 doubles the result.
@feedback 43 | The upper bound is g(1)=1, not 2. A derivative value is not an endpoint image.
@after I=\frac{1}{2}\int_{0}^{1}\cos(u)\,du
@wrong Keep the differential scale and the mapped bounds together.
@why The argument becomes u and x dx becomes du/2; the mapped interval is [0,1] in u.
@step 50 | Choose a primitive H of the complete u-integrand.
@choice 51 | H(u)=\frac{1}{2}\sin(u)
@choice 52 | H(u)=-\frac{1}{2}\sin(u)
@choice 53 | H(u)=\frac{1}{2}\cos(u)
@answer 51
@feedback 52 | Sine differentiates to positive cosine; this option introduces an incorrect minus sign.
@feedback 53 | Cosine differentiates to negative sine, not to the required cosine.
@after H(u)=\frac{1}{2}\sin(u)
@wrong Choose the outer primitive whose derivative is cosine and preserve 1/2.
@why The u-derivative of sin(u)/2 is cos(u)/2, exactly the converted integrand.
@step 60 | Evaluate H(1)-H(0) exactly in radians.
@choice 62 | I=-\frac{1}{2}\sin(1)
@choice 61 | I=\frac{1}{2}\sin(1)
@choice 63 | I=\frac{1}{2}(\sin(1)-1)
@answer 61
@feedback 62 | This reverses the endpoint difference. The upper value is sin(1)/2 and the lower value is zero.
@feedback 63 | sin(0)=0, not 1. Do not transfer the value cos(0)=1 to sine.
@after I=\frac{1}{2}\sin(1)
@wrong Use the sine value at zero and preserve upper-minus-lower order.
@why H(1)=sin(1)/2 and H(0)=sin(0)/2=0. Their difference is sin(1)/2, positive because the angle 1 radian lies between 0 and pi/2.
@step 70 | Check P(x)=H(g(x)) against the original p and P(1)-P(0).
@choice 72 | P'(x)=\frac{1}{2}\cos(x^2),\quad I=\frac{1}{2}\sin(1)
@choice 73 | P'(x)=x\cos(x^2),\quad I=-\frac{1}{2}\sin(1)
@choice 71 | P'(x)=x\cos(x^2),\quad I=\frac{1}{2}\sin(1)
@answer 71
@feedback 72 | This omits the inner derivative 2x. Multiplying it by 1/2 restores the required numerator factor x.
@feedback 73 | The derivative is correct, but P(1)-P(0)=sin(1)/2-0 is positive, not negative.
@after P'(x)=x\cos(x^2),\quad I=\frac{1}{2}\sin(1)
@wrong Check the original x-derivative and each endpoint value separately.
@why P=sin(x squared)/2 gives P'=(1/2)cos(x squared) times 2x=p. At the original limits, P(1)=sin(1)/2 and P(0)=0. The continuous original integrand therefore has the stated signed integral.
@end
