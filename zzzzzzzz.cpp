std::vector<Vector> Sunflower2D(int number, int alpha)
{
    double phi = (1 + sqrt(5)) / 2; // golden ratio
    double angle = 2 * M_PI / pow(phi, 2); // value used to calculate theta for each point
    std::vector<Vector> points;
    float b = round(alpha * sqrt(number)); // number of boundary points
    float theta, r, x, y;
 
    for (int i = 1; i < number + 1; i++) {
        if (i > number - b)
            r = 1.f;
        else
            r = sqrt(i - 0.5) / sqrt(number - (b + 1) / 2);
 
        theta = i * angle;
        x = r * cos(theta);
        y = r * sin(theta);
        points.push_back(Vector(x, y, 0));
    }
    return points;
}
 
bool DoesHitEntity(C_BaseEntity* Target, Vector Point, Vector predicted, float Distance) {
    Vector max = Target->m_vecMaxs();
    predicted.z += max.z / 2;
 
    trace_t tr;
    Ray_t ray;
    ray.Init(Point, predicted);
    CTraceFilterSimple filter(Target, COLLISION_GROUP_PROJECTILE);
    I::EngineTrace->TraceRay(ray, MASK_SOLID, &filter, &tr);
 
    if (!tr.DidHit())
        return true;
 
    return false;
}

bool GetSplashPosition(C_TFPlayer* pLocal, C_BaseEntity* Target, Vector predicted, Vector& out) {
    C_TFWeaponBase* weapon = pLocal->GetActiveTFWeapon();
    if (!weapon)
        return false;
 
    Vector absPos = predicted;
    Vector worldSpaceCenter = absPos;
    worldSpaceCenter.z += Target->m_vecMaxs().z / 2;
 
    float wRadius = G::AttribHookFloat(148, "mult_explosion_radius", weapon, 0, 1);
    auto offset = V::Base.GetOffset(pLocal);
    if (!offset.offset.IsValid() || offset.offset.IsZero())
        return false;
    Vector forward, right, up;
 
    QAngle an = UTIL_CalcAngle(pLocal->EyePosition(), worldSpaceCenter);
    AngleVectors(an, &forward, &right, &up);
 
    Vector shootPos = pLocal->EyePosition() + (forward * offset.offset.x) + (right * offset.offset.y) + (up * offset.offset.z);
 
    std::vector<Vector> goodPoints;
 
    QAngle localAngle = UTIL_CalcAngle(pLocal->EyePosition(), absPos);
    AngleVectors(localAngle, &forward, &right, &up);
 
    std::vector<Vector> optimal = Sunflower2D(200, 2);
 
    float minDist = 10000.0f;
 
    float halfDist = pLocal->EyePosition().DistTo(worldSpaceCenter) * 0.5f;
 
    float displacement = wRadius * 0.5;
 
    std::deque<Vector> newOptimal;
    Vector nf, nr, nu;
 
    for (auto& p : optimal) {
        p = worldSpaceCenter - (forward * halfDist) + (right * p.x * displacement) + (up * p.y * displacement);
 
        AngleVectors(UTIL_CalcAngle(shootPos, p), &nf, &nr, &nu);
 
        Vector end = p + (nf * 8160.f);
 
        trace_t tr;
        Ray_t ray;
        ray.Init(shootPos, end, -offset.size, offset.size);
        CTraceFilterSimple filter(toIgnore, COLLISION_GROUP_PROJECTILE);
        I::EngineTrace->TraceRay(ray, MASK_SOLID, &filter, &tr);
 
        if (!tr.DidHit())
            continue;
 
        float dist = tr.endpos.DistTo(worldSpaceCenter);
 
        if (dist > wRadius)
            continue;
 
        newOptimal.push_back(tr.endpos);
    }
 
    std::sort((newOptimal).begin(), (newOptimal).end(), [&](const Vector& a, const Vector& b) -> bool
        {
            return (a.DistTo(worldSpaceCenter) < b.DistTo(worldSpaceCenter));
        });
 
    for (auto& p : newOptimal) {
        float dist = p.DistTo(worldSpaceCenter);
 
        if (minDist < dist)
            break;
 
        // I::DebugOverlay->AddTextOverlay(tr.endpos, I::GlobalVars->interval_per_tick * 2, "+");
 
        C_BaseEntity* pEnt = nullptr;
        for (CEntitySphereQuery sphere(p, wRadius * 0.9); (pEnt = sphere.GetCurrentEntity()) != nullptr; sphere.NextEntity()) {
            if (pEnt->entindex() == Target->entindex()) {
                if (DoesHitEntity(Target, p, absPos, wRadius)) {
                    minDist = dist;
                    goodPoints.push_back(p);
                }
            }
        }
    }
 
    std::sort((goodPoints).begin(), (goodPoints).end(), [&](const Vector& a, const Vector& b) -> bool
        {
            return (a.DistTo(worldSpaceCenter) < b.DistTo(worldSpaceCenter));
        });
 
    if (goodPoints.empty())
        return false;
 
    I::DebugOverlay->AddBoxOverlay2(goodPoints.front(), -Vector(3, 3, 3), Vector(3, 3, 3), QAngle(0, 0, 0), Color(64, 40, 201, 32), Color(64, 40, 201, 128), I::GlobalVars->interval_per_tick * 2);
 
    out = goodPoints.front();
 
    return true;
}
