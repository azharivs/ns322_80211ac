function y = objective(x)
    global vectPrRates;
    y= sum(sum(x.*vectPrRates));
