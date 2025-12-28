'use strict';

const { draw, makeCylinder, makeBaseBox, makeBox, makeSphere } = replicad;

const defaultParams = {
  handleH: 24,
  outerD: 48,
  squareW: 12 + 0.2,
  squareH: 16 + 0.2
};

const main = (r, $) => {
  let shape

  shape = makeCylinder($.outerD / 2, $.handleH).chamfer(1)

  shape = shape.cut(
    makeBaseBox($.squareW, $.squareW, 16 + $.squareW / 2)
      .chamfer($.squareW / 2 - 0.1, (e) => e.containsPoint([ $.squareW / 2, 0, 16 + $.squareW / 2]))
      .chamfer($.squareW / 2 - 0.1, (e) => e.containsPoint([-$.squareW / 2, 0, 16 + $.squareW / 2]))
      .chamfer($.squareW / 2 - 0.1, (e) => e.containsPoint([0,  $.squareW / 2, 16 + $.squareW / 2]))
      .chamfer($.squareW / 2 - 0.1, (e) => e.containsPoint([0, -$.squareW / 2, 16 + $.squareW / 2]))

  )

  shape = shape.cut(makeSphere(12).translate($.outerD / 2 - 6, 0, $.handleH + 6))

  return [
    {name: 'vernier-handle-v0.1.0', shape, color: '#555', opacity: 1}
  ];
};
