'use strict';

const { draw, makeCylinder, makeBaseBox, makeBox } = replicad;

const defaultParams = {
  outerD: 27,
  moduleD: 44 + 0.6,
  axisDist: 45,
  glassH: 1.10,
  fullH: 11.90 - 2.10,
  innerH: 9.60 - 1.10,
  bearingOuterD: 19 + 0.2, // 698
  bearingInnerD: 8,
  bearingH: 6,
  boltD: 1.6,
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
    $.bearingH * 2,
    [0, 0, $.fullH + $.glassH - $.bearingH - 1]
  ).chamfer(1))


  shape = shape.cut( // sensor hole
    makeBaseBox(23.3, 23.3, 1.7) // PCB hieight
  )

  shape = shape.cut( // sensor hole
    makeBaseBox(23.3 - 4.5, 23.3 - 4.5 , 4) // 3
  )

  shape = [[1,1],[-1,1],[-1,-1],[1,-1]].reduce((res, [x, y]) =>
    res.cut(makeCylinder($.boltD / 2, 10, [x * $.boltOffset, y * $.boltOffset])), shape)

  return [
    {name: 'vernier-basic-v0.2.0', shape, color: '#555', opacity: 1}
  ];
};
