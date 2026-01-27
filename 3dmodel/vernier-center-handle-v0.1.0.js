'use strict';

const { draw, makeCylinder, makeBaseBox, makeBox } = replicad;

const defaultParams = {
  baseH: 12,
  baseD: 46,

  bearingOuterD: 52, // 6808
  bearingInnerD: 40,
  bearingH: 7,

  skrewX: 15.75,
  skrewY: 11.50,

  connBlackX: 16.51,
  connBlackY: 0.3,
  connBlackW: 6,
  connBlackH: 11.5,

  connWhiteX: 7.20,
  connWhiteY: -13.50,
  connWhiteW: 5.5,
  connWhiteH: 5

};

const main = (r, $) => {
  let shape

  // outer body
  shape = makeCylinder($.bearingOuterD / 2 + 2, 21).chamfer(1.5)

  // bearing hole
  shape = shape.cut(makeCylinder($.bearingOuterD / 2 + 0.1, $.bearingH))

  // module hole
  shape = shape.cut(makeCylinder(48 / 2, 21).chamfer(2))

  return [
    {name: 'vernier-center-handle-v0.1.0', shape, color: '#555', opacity: 1}
  ];
};
