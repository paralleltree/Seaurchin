#include "MoverFunctionExpression.h"
#include "Easing.h"

namespace easing {
    bool RegistDefaultMoverFunctionExpressions()
    {
#define SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION(key, name) MoverFunctionExpressionManager::GetInstance().Regist(key, new name ## MoverFunctionExpression())

        return SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("linear", Linear)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("in_quad", InQuad)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("out_quad", OutQuad)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("inout_quad", InOutQuad)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("in_cube", InCubic)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("out_cube", OutCubic)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("inout_cube", InOutCubic)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("in_quart", InQuartic)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("out_quart", OutQuartic)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("inout_quart", InOutQuartic)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("in_quint", InQuintic)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("out_quint", OutQuintic)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("inout_quint", InOutQuintic)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("in_sine", InSinusoidal)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("out_sine", OutSinusoidal)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("inout_sine", InOutSinusoidal)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("in_expo", InExponential)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("out_expo", OutExponential)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("inout_expo", InOutExponential)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("in_circle", InCircular)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("out_circle", OutCircular)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("inout_circle", InOutCircular)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("in_elastic", InElastic)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("out_elastic", OutElastic)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("inout_elastic", InOutElastic)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("in_back", InBack)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("out_back", OutBack)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("inout_back", InOutBack)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("in_bounce", InBounce)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("out_bounce", OutBounce)
            && SU_REGIST_EASING_MOVER_FUNCTION_EXPRESSION("inout_bounce", InOutBounce);
    }
}
