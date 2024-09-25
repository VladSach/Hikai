#include "globals.hlsli"

struct PixelInput {
    float4 pos : SV_Position;
    float4 tc : TEXCOORD0;
};

float4 main(PixelInput input) : SV_Target0
{
    //[KeywordEnum(X, Y, Z)] _Axis ("Plane Axis", Float) = 1.0
    //float _Axis = 1.0;
    float _MajorGridDiv = 10.0; // Range(2,25)

    float _AxisLineWidth = 0.04; // Range(0,1.0)
    float _MajorLineWidth = 0.02; // Range(0,1.0)
    float _MinorLineWidth = 0.01; // Range(0,1.0)

    float4 _MajorLineColor = float4(1, 1, 1, 1);
    float4 _MinorLineColor = float4(1, 1, 1, 1);
    float4 _BaseColor = float4(0, 0, 0, 1);

    float4 _XAxisColor     = float4(1,0,0,1);
    float4 _XAxisDashColor = float4(0.5,0,0,1);
    float4 _YAxisColor     = float4(0,1,0,1);
    float4 _YAxisDashColor = float4(0,0.5,0,1);
    float4 _ZAxisColor     = float4(0,0,1,1);
    float4 _ZAxisDashColor = float4(0,0,0.5,1);
    float _AxisDashScale   = 1.33f;
    float4 _CenterColor    = float4(1,1,1,1);

    PixelInput i = input;

    float4 uvDDXY = float4(ddx(i.tc.xy), ddy(i.tc.xy));
    float2 uvDeriv = float2(length(uvDDXY.xz), length(uvDDXY.yw));

    float axisLineWidth = max(_MajorLineWidth, _AxisLineWidth);
    float2 axisDrawWidth = max(axisLineWidth, uvDeriv);
    float2 axisLineAA = uvDeriv * 1.5;
    float2 axisLines2 = smoothstep(axisDrawWidth + axisLineAA, axisDrawWidth - axisLineAA, abs(i.tc.zw * 2.0));
    axisLines2 *= saturate(axisLineWidth / axisDrawWidth);

    float div = max(2.0, round(_MajorGridDiv));
    float2 majorUVDeriv = uvDeriv / div;
    float majorLineWidth = _MajorLineWidth / div;
    float2 majorDrawWidth = clamp(majorLineWidth, majorUVDeriv, 0.5);
    float2 majorLineAA = majorUVDeriv * 1.5;
    float2 majorGridUV = 1.0 - abs(frac(i.tc.xy / div) * 2.0 - 1.0);
    float2 majorAxisOffset = (1.0 - saturate(abs(i.tc.zw / div * 2.0))) * 2.0;
    majorGridUV += majorAxisOffset; // adjust UVs so center axis line is skipped
    float2 majorGrid2 = smoothstep(majorDrawWidth + majorLineAA, majorDrawWidth - majorLineAA, majorGridUV);
    majorGrid2 *= saturate(majorLineWidth / majorDrawWidth);
    majorGrid2 = saturate(majorGrid2 - axisLines2); // hack
    majorGrid2 = lerp(majorGrid2, majorLineWidth, saturate(majorUVDeriv * 2.0 - 1.0));

    float minorLineWidth = min(_MinorLineWidth, _MajorLineWidth);
    bool minorInvertLine = minorLineWidth > 0.5;
    float minorTargetWidth = minorInvertLine ? 1.0 - minorLineWidth : minorLineWidth;
    float2 minorDrawWidth = clamp(minorTargetWidth, uvDeriv, 0.5);
    float2 minorLineAA = uvDeriv * 1.5;
    float2 minorGridUV = abs(frac(i.tc.xy) * 2.0 - 1.0);
    minorGridUV = minorInvertLine ? minorGridUV : 1.0 - minorGridUV;
    float2 minorMajorOffset = (1.0 - saturate((1.0 - abs(frac(i.tc.zw / div) * 2.0 - 1.0)) * div)) * 2.0;
    minorGridUV += minorMajorOffset; // adjust UVs so major division lines are skipped
    float2 minorGrid2 = smoothstep(minorDrawWidth + minorLineAA, minorDrawWidth - minorLineAA, minorGridUV);
    minorGrid2 *= saturate(minorTargetWidth / minorDrawWidth);
    minorGrid2 = saturate(minorGrid2 - axisLines2); // hack
    minorGrid2 = lerp(minorGrid2, minorTargetWidth, saturate(uvDeriv * 2.0 - 1.0));
    minorGrid2 = minorInvertLine ? 1.0 - minorGrid2 : minorGrid2;
    minorGrid2 = select(abs(i.tc.zw) > 0.5, minorGrid2, 0.0);

    half minorGrid = lerp(minorGrid2.x, 1.0, minorGrid2.y);
    half majorGrid = lerp(majorGrid2.x, 1.0, majorGrid2.y);

    float2 axisDashUV = abs(frac((i.tc.zw + axisLineWidth * 0.5) * _AxisDashScale) * 2.0 - 1.0) - 0.5;
    float2 axisDashDeriv = uvDeriv * _AxisDashScale * 1.5;
    float2 axisDash = smoothstep(-axisDashDeriv, axisDashDeriv, axisDashUV);
    axisDash = select(i.tc.zw < 0.0, axisDash, 1.0);

    half4 xAxisColor = _XAxisColor;
    half4 yAxisColor = _YAxisColor;
    half4 zAxisColor = _ZAxisColor;
    half4 xAxisDashColor = _XAxisDashColor;
    half4 yAxisDashColor = _YAxisDashColor;
    half4 zAxisDashColor = _ZAxisDashColor;
    half4 centerColor = _CenterColor;
    half4 majorLineColor = _MajorLineColor;
    half4 minorLineColor = _MinorLineColor;
    half4 baseColor = _BaseColor;

    half4 aAxisColor = xAxisColor;
    half4 bAxisColor = yAxisColor;
    half4 aAxisDashColor = xAxisDashColor;
    half4 bAxisDashColor = yAxisDashColor;

    aAxisColor = lerp(aAxisDashColor, aAxisColor,
                      axisDash.y);
    bAxisColor = lerp(bAxisDashColor, bAxisColor,
                      axisDash.x);
    aAxisColor = lerp(aAxisColor, centerColor,
                      axisLines2.y);

    half4 axisLines = lerp(bAxisColor * axisLines2.y, aAxisColor, axisLines2.x);

    half4 col = lerp(baseColor, minorLineColor, minorGrid *  minorLineColor.a);
    col = lerp(col, majorLineColor, majorGrid * majorLineColor.a);
    col = col * (1.0 - axisLines.a) + axisLines;

    return col;
}
