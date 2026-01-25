'use strict';

const { draw, makeCylinder, makeBaseBox, makeBox } = replicad;

const defaultParams = {
  outerD: 28,
  moduleD: 44 + 0.6,
  axisDist: 45,
  glassH: 1.10,
  fullH: 11.90 - 2.10,
  innerH: 9.60 - 1.10,
  bearingOuterD: 19 + 0.1, // 698
  bearingInnerD: 8,
  bearingH: 6,
  boltD: 2,
  boltOffset: 17 / 2
};

const main = (r, $) => {
  let shape

  shape = (makeCylinder( // handle piedestal
    $.outerD / 2,
    $.fullH
  ))

  shape = shape.cut(makeCylinder( // outer bearing hole
    $.bearingOuterD / 2,
    $.bearingH,
    [0, 0, $.fullH + $.glassH - $.bearingH]
  ))

  shape = shape.cut(makeCylinder( // under bearing hole
    $.bearingOuterD / 2 - 1.5,
    $.fullH
  ))

  shape = shape.cut( // sensor hole
    makeBaseBox(23.4, 23.4, 5)
      .chamfer(3, (e) => e.containsPoint([ 11.7, 0, 5]))
      .chamfer(3, (e) => e.containsPoint([-11.7, 0, 5]))
      .chamfer(3, (e) => e.containsPoint([0,  11.7, 5]))
      .chamfer(3, (e) => e.containsPoint([0, -11.7, 5]))

  )

  shape = [[1,1],[-1,1],[-1,-1],[1,-1]].reduce((res, [x, y]) =>
    res.cut(makeCylinder(1, 10, [x * $.boltOffset, y * $.boltOffset])), shape)

  return [
    {name: 'vernier-basic-v0.1.0', shape, color: '#555', opacity: 1}
  ];
};
