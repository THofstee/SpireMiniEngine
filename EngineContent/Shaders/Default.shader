module DefaultGeometry implements IMaterialGeometry
{
    require vec3 coarseVertPos;
    public using NoTessellation;
    public vec3 displacement = vec3(0.0);
}

module DefaultPattern implements IMaterialPattern
{
    public vec3 albedo = vec3(0.9, 0.45, 0.05);
    public vec3 normal = vec3(0.0, 0.0, 1.0);
    public float specular = 0.5;
    public float roughness = 0.7;
    public float metallic = 0.4;
    public float selfShadow(vec3 lightDir)
    {
        return 1.0;
    }
}