'use strict';

const { draw, makeCylinder, makeBaseBox, makeBox, makeSphere } = replicad;

const defaultParams = {
  handleH: 24,
  outerD: 48,
  innerD: 32,
  squareW: 12 + 0.2,
  squareH: 6 + 0.2
};

const main = (r, $) => {
  let shape

  shape = makeCylinder($.outerD / 2, $.handleH).chamfer(2)

  shape = shape.cut(
    makeBaseBox($.squareW, $.squareW, 16 + $.squareH)
  )

  shape = shape.cut(makeCylinder($.innerD / 2, 12))

  // shape = shape.cut(makeSphere(12).translate($.outerD / 2 - 6, 0, $.handleH + 6))

  return [
    {name: 'vernier-handle-v0.2.0', shape, color: '#555', opacity: 1}
  ];
};
