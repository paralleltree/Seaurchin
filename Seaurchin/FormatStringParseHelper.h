#pragma once

#include "ScriptSprite.h"
#include "ScriptSpriteMover.h"

#define SKIP_SP(p) while(*p != '\0' && !isgraph(*p)) ++p
#define CHECK_END(p) do { if (*p == '\0') return false; } while(0)

namespace FormatStringParseHelper {
    using namespace std;

    inline bool Double(const char *expression, const char **seeked, double &retVal)
    {
        const char *p = expression;

        SKIP_SP(p);
        CHECK_END(p);

        bool isPositive = false, isNegative = false;
        if (*p == '+') {
            isPositive = true;
            ++p;
        } else if (*p == '-') {
            isNegative = true;
            ++p;
        }

        if (isdigit(*p) || *p == '.') {
            double val = 0.0;
            while (isdigit(*p)) {
                val = val * 10.0 + (*p - '0');
                ++p;
            }
            if (*p == '.') {
                ++p;

                double dig = 0.1;
                while (isdigit(*p)) {
                    val += (*p - '0') * dig;
                    dig *= 0.1;
                    ++p;
                }
            }

            if (isNegative) val *= -1.0;

            retVal = val;
            *seeked = p;
            return true;
        }

        return false;
    }
    inline bool String(const char *expression, const char **seeked, string &retVal)
    {
        const char *p = expression;

        SKIP_SP(p);
        CHECK_END(p);

        if (isalpha(*p) || *p == '_' || *p == '@') {
            const char *head = p;
            do ++p; while (isalnum(*p) || *p == '_' || *p == '@');
            const char *tail = p;
            const size_t len = tail - head;

            *seeked = p;
            retVal = string(head, len);
            return true;
        }

        return false;
    }
    inline bool FieldID(const char *expression, const char **seeked, SSprite::FieldID &retVal)
    {
        const char *p = expression;

        SKIP_SP(p);
        CHECK_END(p);

        if (isalpha(*p)) {
            const char *head = p;
            do ++p; while (isalnum(*p));
            const char *tail = p;
            const size_t len = tail - head;

            {
                char *buf = (char *)malloc(sizeof(char) * (len + 1));
                if (buf == NULL) return false;

                memcpy(buf, head, len);
                buf[len] = '\0';

                *seeked = p;
                retVal = SSprite::GetFieldId(buf);
                free(buf);
            }
            return true;
        }

        return false;
    }

    inline bool ApplyPair(const char *expression, const char **seeked, SSprite::FieldID &key, double &dVal)
    {
        const char *p = expression;

        if (!FieldID(p, &p, key)) return false;

        SKIP_SP(p);
        CHECK_END(p);

        if (*p != ':') return false;
        ++p;
        CHECK_END(p);

        {
            double dv;
            if (Double(p, &p, dv)) {
                *seeked = p;
                dVal = dv;
                return true;
            }
        }

        return false;
    }
    inline bool Apply(const char *expression, SSprite* pSprite)
    {
        const char *p = expression;

        SKIP_SP(p);

        while (*p != '\0') {
            SSprite::FieldID key;
            double dVal;

            if (!ApplyPair(p, &p, key, dVal)) return false;

            if (!pSprite->Apply(key, dVal)) {
                // NOTE: Applyがログ出すからそれでいいかな
            }

            SKIP_SP(p);

            if (*p == '\0') break;
            if (*p != ',') return false;

            ++p;
            CHECK_END(p);
        }

        return true;
    }

    inline bool AddMoveObjectPair(const char *expression, const char **seeked, string &key, double &dVal, string &sVal, bool &isStr)
    {
        const char *p = expression;

        if (!String(p, &p, key)) return false;

        SKIP_SP(p);
        CHECK_END(p);

        if (*p != ':') return false;
        ++p;
        CHECK_END(p);

        {
            double dv;
            if (Double(p, &p, dv)) {
                *seeked = p;
                isStr = false;
                dVal = dv;
                return true;
            }
        }

        {
            string sv;
            if (String(p, &p, sv)) {
                *seeked = p;
                isStr = true;
                sVal = sv;
                return true;
            }
        }

        return false;
    }
    inline bool AddMoveObject(const char *expression, const char **seeked, MoverObject* pMoverObject)
    {
        const char *p = expression;

        SKIP_SP(p);

        while (*p != '\0') {
            string key;
            double dVal;
            string sVal;
            bool isStr;

            if (!AddMoveObjectPair(p, &p, key, dVal, sVal, isStr)) return false;

            if (isStr) {
                if (!pMoverObject->Apply(key, sVal)) {
                    // NOTE: Applyがログ出すからそれでいいかな
                }
            } else {
                if (!pMoverObject->Apply(key, dVal)) {
                    // NOTE: Applyがログ出すからそれでいいかな
                }
            }

            SKIP_SP(p);

            if (*p == '\0') break;
            if (*p != ',') break;

            ++p;
            CHECK_END(p);
        }

        *seeked = p;
        return true;
    }
    inline bool AddMovePair(const char *expression, const char **seeked, SSpriteMover* pMover)
    {
        const char *p = expression;
        vector<SSprite::FieldID> fields; // NOTE: 8個も取っておけばそうそうリサイズされないでしょという気持ち

        SKIP_SP(p);
        CHECK_END(p);

        do {
            SSprite::FieldID id;
            if (!FieldID(p, &p, id)) return false;
            fields.emplace_back(id);

            SKIP_SP(p);
            CHECK_END(p);

            if (*p == ':') {
                ++p;
                break;
            }
            if (*p != ',') return false;

            ++p;
            CHECK_END(p);
        } while (*p != '\0');

        SKIP_SP(p);
        CHECK_END(p);

        if (*p != '{') return false;
        ++p;

        SKIP_SP(p);
        CHECK_END(p);

        MoverObject *pMoverObject = new MoverObject();
        bool retVal = false;
        while (*p != '\0') {
            if (!AddMoveObject(p, &p, pMoverObject)) break;

            SKIP_SP(p);
            if (*p == '\0') break;

            if (*p == '}') {
                ++p;
                retVal = true;
                break;
            }
            if (*p != ',') break;

            ++p;
            SKIP_SP(p);
            if (*p == '}') break;
        }

        if (!retVal) {
            pMoverObject->Release();
            return false;
        }

        for (auto it = fields.begin(); it != fields.end(); ++it) {
            // TODO: 同じパラメータを持つ異なるフィールドのMoverObjectを複数登録するのではなく、MoverObjectが複数のフィールドを一括制御できるようにする
            MoverObject *pTmpMoverObj = pMoverObject->Clone();
            pTmpMoverObj->RegistTargetField(*it);
            pMover->AddMove(pTmpMoverObj);
        }
        pMoverObject->Release();

        *seeked = p;
        return true;
    }
    inline bool AddMove(const char *expression, SSpriteMover* pMover)
    {
        const char *p = expression;

        while (*p != '\0') {
            if (!AddMovePair(p, &p, pMover)) return false;

            if (*p == '\0') break;
            SKIP_SP(p);

            if (*p == '\0') break;
            if (*p != ',') return false;

            ++p;
            CHECK_END(p);
        }

        return true;
    }
}

#undef SKIP_SP
#undef CHECK_END
