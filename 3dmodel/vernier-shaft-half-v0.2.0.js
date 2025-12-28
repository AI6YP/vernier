'use strict';

const { draw, makeCylinder, makeBaseBox, makeBox } = replicad;

const defaultParams = {
  magnetD: 4 + 0.4,
  magnetH: 2 + 0.5,

  shaftH: 8,

  bearingOuterD: 22 + 0.4, // 608
  bearingInnerD: 8 - 0.3,
  bearingH: 7,
};

const main = (r, $) => {
  let shape

  shape = makeCylinder($.bearingInnerD / 2, $.shaftH + 5).chamfer(1)

  shape = shape.fuse(
    makeCylinder($.bearingInnerD / 2 + 1, $.shaftH, [0,0,$.shaftH]).chamfer(0.5)
  )

  shape = shape.fuse(
    makeBaseBox(6,11.7,16).translate(3, 0, $.shaftH + 1).chamfer(0.5)
  )

  shape = shape.cut(makeCylinder($.magnetD / 2, $.magnetH).chamfer(0.3))

  shape = shape.cut(makeBaseBox(60,60,60).translateX(-30))

  return [
    {name: 'vernier-shaft-half-v0.2.0', shape, color: '#555', opacity: 1}
  ];
};
